#include "cpplox/Bytecode/VM.h"

#include <iomanip>

#include "cpplox/Bytecode/Compiler.h"
#include "cpplox/Bytecode/common.h"

namespace vm {

// TODO: Put asserts in #defines

InterpretResult VM::interpret(std::unique_ptr<Function> in_func) {
  if (already_called) {
    throw std::logic_error(
        "VM not designed to be called multiple times, create a new instance.");
  }
  std::shared_ptr<Function> func = std::move(in_func);
  stack.push_back(func);
  call(stack.back(), 0);
  already_called = true;

#ifdef DEBUG_TRACE_EXECUTION
  disassembler.disassemble_constants_table(*curr_fun->chunk);
  log_output << "=== execution ===" << std::endl;
#endif
  auto ret = run();
#ifdef DEBUG_TRACE_EXECUTION
  log_output << "==/ execution /==" << std::endl;
#endif

  return ret;
}

InterpretResult VM::run() {
#define READ_CODE() (curr_fun->chunk->code[curr_frame->ip++])
#define READ_UINT16()                        \
  (static_cast<uint16_t>(READ_CODE()) << 8 | \
   static_cast<uint16_t>(READ_CODE()));
#define BINARY_OP(op)                                                   \
  do {                                                                  \
    if (!std::holds_alternative<double>(stack.back()) ||                \
        !std::holds_alternative<double>(stack.at(stack.size() - 2))) {  \
      set_runtime_error("Operands must be numbers.");                   \
      return InterpretResult::INTERPRET_RUNTIME_ERROR;                  \
    }                                                                   \
    double rhs = std::get<double>(stack.back());                        \
    stack.pop_back();                                                   \
    double lhs = std::get<double>(stack.back());                        \
    stack.back() = Value(lhs op rhs);                                   \
  } while (false)

  while (1) {
    OpCode opcode{READ_CODE()};

#ifdef DEBUG_TRACE_EXECUTION
    trace_execution();
#endif

    switch (opcode) {
      case OpCode::OP_JUMP_IF_FALSE: {
        uint16_t offset = READ_UINT16();
        if (is_falsey(stack.back())) curr_frame->ip += offset;
        break;
      }
      case OpCode::OP_JUMP: {
        uint16_t offset = READ_UINT16();
        curr_frame->ip += offset;
        break;
      }
      case OpCode::OP_LOOP: {
        uint16_t offset = READ_UINT16();
        curr_frame->ip += -offset;
        break;
      }
      case OpCode::OP_PRINT:
        output << value::to_string(stack.back()) << std::endl;
        stack.pop_back();
        break;
      case OpCode::OP_RETURN: {
        const Value return_value = stack.back();
        stack.pop_back();
        // Hold on to return value as VM discards function's stack window 
        // (shared between caller [as arguments] and callee [as parameters]).
        // By value instead of ref as .pop_back() would invalidate the
        // reference.
        if (call_frames.size() == 1) {
          call_frames.pop_back();
          assert(stack.size() == 1);
          stack.pop_back();
          return InterpretResult::INTERPRET_OK;
          // This was top-level function.
        }
        assert(stack.size() - curr_fun->arity - 1 > 0);
        stack.erase(stack.end() - 1 - curr_fun->arity, stack.end());
        stack.push_back(return_value);
        call_frames.pop_back();
        update_frame_pointers();
        break;
      }
      case OpCode::OP_CONSTANT:
        stack.push_back(curr_fun->chunk->constants[READ_CODE()]);
        break;
      case OpCode::OP_NIL:
        stack.push_back(std::monostate());
        break;
      case OpCode::OP_TRUE:
        stack.push_back(true);
        break;
      case OpCode::OP_FALSE:
        stack.push_back(false);
        break;
      case OpCode::OP_POP:
        stack.pop_back();
        break;
      case OpCode::OP_NOOP:
        curr_frame->ip += 1;
        break;
      case OpCode::OP_CALL: {
        uint8_t arg_count = READ_CODE();
        const Value& maybe_function_to_run =
            stack[stack.size() - 1 - arg_count];
        if (!call(maybe_function_to_run, arg_count)) {
          return InterpretResult::INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OpCode::OP_GET_LOCAL: {
        size_t idx = READ_CODE() + curr_frame->stack_offset;
        assert(idx < stack.size());
        stack.push_back(stack[idx]);
        // This VM is stack-based and other instructions can only take data from
        // stack top. Register-based VM could fetch data directly by idx at the
        // cost of larger instructions.
        break;
      }
      case OpCode::OP_SET_LOCAL: {
        size_t idx = READ_CODE() + curr_frame->stack_offset;
        assert(idx < stack.size());
        stack[idx] = stack.back();
        // No .pop_back() as assignment is an expression and has to produce a value.
        // In this case is the assigned value itself, eg. > print a = 8; prints "8".
        break;
      }
      case OpCode::OP_GET_GLOBAL: {
        const Value& maybe_global_var_name =
            curr_fun->chunk->constants[READ_CODE()];
        if (!std::holds_alternative<std::string>(maybe_global_var_name)) {
          set_runtime_error("Global variable name loading error, expected string");
          return InterpretResult::INTERPRET_RUNTIME_ERROR;
        }
        const std::string global_var_name =
            std::get<std::string>(maybe_global_var_name);
        auto search = globals.find(global_var_name);
        if (search == globals.end()) {
          set_runtime_error("Undefined variable '" + global_var_name + "'.");
          return InterpretResult::INTERPRET_RUNTIME_ERROR;
        }
        stack.push_back(search->second);
        // TODO: Even though globals can be redefined, could it be ok to add
        // Value by reference to stack
        //       (assuming relevant typing changes are doable)? Thinking is that
        //       any update to this global variable would happen in an
        //       assignment, by which time this Value will no longer be on
        //       stack. Think through. Alternatively: this is only a problem
        //       when copying is expensive - what cases apart from string are
        //       costly? If none - could string interning address this (then
        //       only a handle to large object is copied)?
        break;
      }
      case OpCode::OP_DEFINE_GLOBAL: {
        const Value& maybe_global_var_name =
            curr_fun->chunk->constants[READ_CODE()];
        if (!std::holds_alternative<std::string>(maybe_global_var_name)) {
          set_runtime_error(
              "Global variable name loading error, expected string");
          return InterpretResult::INTERPRET_RUNTIME_ERROR;
        }
        globals.insert_or_assign(std::get<std::string>(maybe_global_var_name),
                                 stack.back());
        stack.pop_back();
        break;
      }
      case OpCode::OP_SET_GLOBAL: {
        const Value& maybe_var_name = curr_fun->chunk->constants[READ_CODE()];
        if (!std::holds_alternative<std::string>(maybe_var_name)) {
          set_runtime_error(
              "Global variable name loading error, expected string");
          return InterpretResult::INTERPRET_RUNTIME_ERROR;
        }
        const std::string var_name = std::get<std::string>(maybe_var_name);
        auto search = globals.find(var_name);
        if (search == globals.end()) {
          set_runtime_error("Undefined variable '" + var_name + "'.");
          return InterpretResult::INTERPRET_RUNTIME_ERROR;
        }
        globals[var_name] = stack.back();
        // No .pop_back() as assignment is an expression and has to produce a value.
        // In this case is the assigned value itself, eg. > print a = 8; prints "8".
        break;
      }
      case OpCode::OP_EQUAL: {
        const Value rhs = stack.back();
        stack.pop_back();
        // By value instead of ref as .pop_back() would invalidate the
        // reference.
        const Value lhs = stack.back();
        stack.back() = values_equal(lhs, rhs);
        break;
      }
      case OpCode::OP_GREATER:
        BINARY_OP(>);
        break;
      case OpCode::OP_LESS:
        BINARY_OP(<);
        break;
      case OpCode::OP_ADD: {
        // TODO: can it be summarised into a single condition?
        // Both alternatives offer the same interface for +.
        // what is better - variant visitor/some sort of auto/explicit
        // specialisation?
        const Value rhs = stack.back();
        stack.pop_back();
        const Value lhs = stack.back();
        // By value instead of ref as .pop_back() would invalidate the
        // reference.
        if (type_match<std::string>(lhs, rhs)) {
          stack.back() = add<std::string>(lhs, rhs);
        } else if (type_match<double>(lhs, rhs)) {
          stack.back() = add<double>(lhs, rhs);
        } else {
          set_runtime_error("Operands must be two numbers or strings.");
          return InterpretResult::INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OpCode::OP_SUBTRACT:
        BINARY_OP(-);
        break;
      case OpCode::OP_MULTIPLY:
        BINARY_OP(*);
        break;
      case OpCode::OP_DIVIDE:
        BINARY_OP(/);
        break;
      case OpCode::OP_NOT:
        stack.back() = is_falsey(stack.back());
        break;
      case OpCode::OP_NEGATE:
        if (!std::holds_alternative<double>(stack.back())) {
          set_runtime_error("Operand must be a number.");
          return InterpretResult::INTERPRET_RUNTIME_ERROR;
        }
        stack.back() = -std::get<double>(stack.back());
        break;
      default:
        return InterpretResult::INTERPRET_RUNTIME_ERROR;
    }
  }

#undef BINARY_OP
#undef READ_UINT16
#undef READ_CODE
}

bool VM::call(const Value& maybe_callable, uint8_t arg_count) {
  // TODO: Is this the cleanest way to pass Value (shared_ptr<Function> for now)?
  // See if any way to refactor to make the contract explicit (Function owned
  // by Value that's in constants).
  if (call_frames.size() >= MAX_CALLSTACK_DEPTH) {
    set_runtime_error("Stackoverflow.");
    return false;    
  }
  if (!std::holds_alternative<std::shared_ptr<Function>>(maybe_callable)) {
    set_runtime_error("Only Function objects are callable");
    return false;
  }
  const Function& f =
      *std::get<std::shared_ptr<Function>>(maybe_callable).get();
  if (arg_count != f.arity) {
    set_runtime_error("Function " + f.name + " expected " +
                          std::to_string(f.arity) + " parameters, but got " +
                          std::to_string(arg_count) + ".");
    return false;
  }
  CallFrame cf{f, 0, stack.size() -1 - arg_count};
  // stack_offset points to the Value of the invoked function.
  call_frames.push_back(cf);
  update_frame_pointers();
  return true;
}

void VM::update_frame_pointers() {
  curr_frame = &call_frames.back();
  curr_fun = &curr_frame->function;
}

bool VM::is_falsey(const Value& val) const {
  if (std::holds_alternative<std::monostate>(val)) return true;
  if (std::holds_alternative<bool>(val) && !std::get<bool>(val)) return true;
  return false;
}

bool VM::values_equal(const Value& lhs, const Value& rhs) const {
  return lhs == rhs;  // 2 std::monostate variants compare equal.
}

void VM::set_runtime_error(std::string err_msg) const {
  for (auto it = call_frames.crbegin(); it != call_frames.crend(); it++) {
    std::string call_site_line = std::to_string(it->function.chunk->line_numbers[it->ip-1]);
    std::cerr << "[line " + call_site_line + "] in " + it->function.name << std::endl;
    // TODO: Consider taking this stream as constructor param.
  }
  int line = curr_fun->chunk->line_numbers[curr_frame->ip];
  e_reporter.set_error("[Runtime error] [line " + std::to_string(line) +
                       "] while interpreting: " + err_msg);
}

void VM::trace_execution() const {
  log_output << "       Stack: ";
  if (stack.size() == 0) {
    log_output << "[ ]";
  } else {
    for (const auto& val : stack) {
      log_output << "[ " << value::to_string(val) << " ]";
    }
  }
  log_output << std::endl;
  log_output.flush();
  disassembler.disassemble_instruction(*curr_fun->chunk, curr_frame->ip - 1);
}

template <typename T>
bool VM::type_match(const Value& lhs, const Value& rhs) {
  return std::holds_alternative<T>(lhs) && std::holds_alternative<T>(rhs);
}
template <typename T>
T VM::add(const Value& lhs, const Value& rhs) {
  return std::get<T>(lhs) + std::get<T>(rhs);
}

}  // namespace vm