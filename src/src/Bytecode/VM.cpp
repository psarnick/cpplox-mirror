#include "cpplox/Bytecode/VM.h"

#include <iomanip>

#include "cpplox/Bytecode/Compiler.h"
#include "cpplox/Bytecode/common.h"

namespace vm {

/*
        TODO: check if just popping from the stack deletes the underlying
   memory? https://craftinginterpreters.com/strings.html 'freeing objects' I
   think the answer is yes, but an interesting exercise

        Implement string interning
*/

InterpretResult VM::interpret(const Chunk& chunk) {
  if (already_called) {
    throw std::logic_error(
        "VM not designed to be called multiple times, create a new instance.");
  }
  already_called = true;
  ip = 0;
  disassembler.disassemble_constants_table(chunk);
#ifdef DEBUG_TRACE_EXECUTION
  log_output << "=== execution ===" << std::endl;
#endif
  auto ret = run(chunk);
#ifdef DEBUG_TRACE_EXECUTION
  log_output << "==/ execution /==" << std::endl;
#endif
  return ret;
}

InterpretResult VM::run(const Chunk& chunk) {
#define BINARY_OP(op)                                                         \
  do {                                                                        \
    if (!std::holds_alternative<double>(stack.back()) ||                      \
        !std::holds_alternative<double>(stack.at(stack.size() - 2))) {        \
      set_runtime_error("Operands must be numbers.", chunk.line_numbers[ip]); \
      return InterpretResult::INTERPRET_RUNTIME_ERROR;                        \
    }                                                                         \
    double rhs = std::get<double>(stack.back());                              \
    stack.pop_back();                                                         \
    double lhs = std::get<double>(stack.back());                              \
    stack.back() = Value(lhs op rhs);                                         \
  } while (false)

  while (1) {
    OpCode opcode{chunk.code[ip++]};
#ifdef DEBUG_TRACE_EXECUTION
    log_output << "       Stack: ";
    if (stack.size() == 0) {
      log_output << "[ ]";
    } else {
      for (const auto& val : stack) {
        log_output << "[ " << value::to_string(val) << " ]";
      }
    }
    log_output << std::endl;
    disassembler.disassemble_instruction(chunk, ip - 1);
#endif
    switch (opcode) {
      case OpCode::OP_JUMP_IF_FALSE: {
        uint16_t offset = static_cast<uint16_t>(chunk.code[ip]) << 8 | static_cast<uint16_t>(chunk.code[ip+1]);
        ip += 2;
        if (is_falsey(stack.back())) ip += offset;
        break;
      }
      case OpCode::OP_JUMP: {
        uint16_t offset = static_cast<uint16_t>(chunk.code[ip]) << 8 | static_cast<uint16_t>(chunk.code[ip+1]);
        ip += offset + 2;
        break;
      }
      case OpCode::OP_LOOP: {
        uint16_t offset = static_cast<uint16_t>(chunk.code[ip]) << 8 | static_cast<uint16_t>(chunk.code[ip+1]);
        ip += -offset + 2;
        break;
      }
      case OpCode::OP_PRINT:
        output << value::to_string(stack.back()) << std::endl;
        stack.pop_back();
        break;
      case OpCode::OP_RETURN:
        return InterpretResult::INTERPRET_OK;
      case OpCode::OP_CONSTANT:
        stack.push_back(chunk.constants[chunk.code[ip++]]);
        // After opcode is read above, chunk.code[ip] points to
        // the relevant constant's index in the pool.
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
      case OpCode::OP_GET_LOCAL: {
        uint8_t idx = chunk.code[ip++];
        assert(idx < stack.size());
        stack.push_back(stack[idx]);
        // VM is stack-based and other instructions can only take data from
        // stack top. Register-based VM could fetch data directly by idx at the
        // cost of larger instructions.
        break;
      }
      case OpCode::OP_SET_LOCAL: {
        uint8_t idx = chunk.code[ip++];
        assert(idx < stack.size());
        stack[idx] = stack.back();
        // No .pop() as assignment is an expression and has to produce a value,
        // which in its case is the assigned value itself.
        break;
      }
      case OpCode::OP_GET_GLOBAL: {
        const Value& maybe_global_var_name = chunk.constants[chunk.code[ip++]];
        if (!std::holds_alternative<std::string>(maybe_global_var_name)) {
          set_runtime_error(
              "Global variable name loading error, expected string",
              chunk.line_numbers[ip]);
          return InterpretResult::INTERPRET_RUNTIME_ERROR;
        }
        const std::string global_var_name =
            std::get<std::string>(maybe_global_var_name);
        auto search = globals.find(global_var_name);
        if (search == globals.end()) {
          set_runtime_error("Undefined variable '" + global_var_name + "'.",
                            chunk.line_numbers[ip]);
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
        const Value& maybe_global_var_name = chunk.constants[chunk.code[ip++]];
        // Return string stored under operand's index in constants table.
        if (!std::holds_alternative<std::string>(maybe_global_var_name)) {
          set_runtime_error(
              "Global variable name loading error, expected string",
              chunk.line_numbers[ip]);
          return InterpretResult::INTERPRET_RUNTIME_ERROR;
        }
        globals.insert_or_assign(std::get<std::string>(maybe_global_var_name),
                                 stack.back());
        stack.pop_back();
        break;
      }
      case OpCode::OP_SET_GLOBAL: {
        const Value& maybe_var_name = chunk.constants[chunk.code[ip++]];
        if (!std::holds_alternative<std::string>(maybe_var_name)) {
          set_runtime_error(
              "Global variable name loading error, expected string",
              chunk.line_numbers[ip]);
          return InterpretResult::INTERPRET_RUNTIME_ERROR;
        }
        const std::string var_name = std::get<std::string>(maybe_var_name);
        auto search = globals.find(var_name);
        if (search == globals.end()) {
          set_runtime_error("Undefined variable '" + var_name + "'.",
                            chunk.line_numbers[ip]);
          return InterpretResult::INTERPRET_RUNTIME_ERROR;
        }
        globals[var_name] = stack.back();
        // No .pop_back() as assignment is an expression, so needs to leave
        // value on stack in case of nesting, > print a = 8; prints "8".
        break;
      }
      case OpCode::OP_EQUAL: {
        const Value rhs = stack.back();
        stack.pop_back();
        // By value istead of ref as stack.pop_back() would invalidate the
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
        // By value istead of ref as stack.pop_back() would invalidate the
        // reference.
        if (type_match<std::string>(lhs, rhs)) {
          stack.back() = add<std::string>(lhs, rhs);
        } else if (type_match<double>(lhs, rhs)) {
          stack.back() = add<double>(lhs, rhs);
        } else {
          set_runtime_error("Operands must be two numbers or strings.",
                            chunk.line_numbers[ip]);
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
          set_runtime_error("Operand must be a number.",
                            chunk.line_numbers[ip]);
          return InterpretResult::INTERPRET_RUNTIME_ERROR;
        }
        stack.back() = -std::get<double>(stack.back());
        break;
      default:
        return InterpretResult::INTERPRET_RUNTIME_ERROR;
    }
  }
#undef BINARY_OP
}

bool VM::is_falsey(const Value& val) const {
  if (std::holds_alternative<std::monostate>(val)) return true;
  if (std::holds_alternative<bool>(val) && !std::get<bool>(val)) return true;
  return false;
}

bool VM::values_equal(const Value& lhs, const Value& rhs) const {
  return lhs == rhs;  // 2 std::monostate variants compare equal.
}

void VM::set_runtime_error(std::string err_msg, int line) const {
  e_reporter.set_error("[Runtime error] [line " + std::to_string(line) +
                       "] while interpreting: " + err_msg);
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