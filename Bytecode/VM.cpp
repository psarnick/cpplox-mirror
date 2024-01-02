#include <iomanip>

#include "cpplox/Bytecode/VM.h"
#include "cpplox/Bytecode/Compiler.h"
#include "cpplox/Bytecode/common.h"

namespace cpplox {

  InterpretResult VM::interpret(function_ptr in_func) {
    if (already_called) {
      throw std::logic_error(
        "VM not designed to be called multiple times, create a new instance.");
    }
    stack.push_back(heap->make<Closure>(in_func));
    call(0);
    already_called = true;

  #ifdef DEBUG_TRACE_EXECUTION
    log_output << "=== execution ===" << std::endl;
  #endif
    InterpretResult ret = run();
  #ifdef DEBUG_TRACE_EXECUTION
    log_output << "==/ execution /==" << std::endl;
  #endif

    return ret;
  }

  InterpretResult VM::run() {
  #define READ_CODE() (curr_fun->chunk->code[curr_frame->ip++])
  #define READ_UINT16()                                                  \
    (static_cast<uint16_t>(READ_CODE()) << 8 |                           \
    static_cast<uint16_t>(READ_CODE()));
  #define BINARY_OP(op)                                                  \
    do {                                                                 \
      if (!std::holds_alternative<double>(*(stack.end() - 1)) ||         \
          !std::holds_alternative<double>(*(stack.end() - 2))) {         \
        set_runtime_error("Operands must be numbers.");                  \
        return InterpretResult::INTERPRET_RUNTIME_ERROR;                 \
      }                                                                  \
      double rhs = std::get<double>(stack.back());                       \
      stack.pop_back();                                                  \
      stack.back() = Value(std::get<double>(stack.back()) op rhs);       \
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
          output << to_string(stack.back()) << std::endl;
          stack.pop_back();
          break;
        case OpCode::OP_RETURN: {
          close_upvalues(curr_frame->stack_offset + 1);
          if (call_frames.size() == 1) {
            call_frames.pop_back();
            assert(stack.size() == 2);
            // This is top-level function and stack contains return value & main script.
            stack.clear();
            return InterpretResult::INTERPRET_OK;
          }
          stack[curr_frame->stack_offset] = stack.back();
          stack.erase(stack.begin() + curr_frame->stack_offset + 1, stack.end());
          // Instead of: capture return value in a local variable, .erase() to clean function's
          // stack window and then .push_back() to store the return value, writing directly to the 
          // base of current closure's stack window. This is done to ensure return value is always
          // reachable from stack to prevent GC from collecting it. +1 to keep return value.
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
        case OpCode::OP_CLOSE_UPVALUE: {
          close_upvalues(stack.size()-1);
          stack.pop_back();
          // TODO: Think through GC - is this GC safe?
          break;
        }
        case OpCode::OP_NOOP:
          curr_frame->ip += 1;
          break;
        case OpCode::OP_CALL: {
          uint8_t arg_count = READ_CODE();
          if (!call(arg_count)) {
            return InterpretResult::INTERPRET_RUNTIME_ERROR;
          }
          break;
        }
        case OpCode::OP_CLOSURE: {
          const Value maybe_function_ptr = curr_fun->chunk->constants[READ_CODE()];
          if (!std::holds_alternative<function_ptr>(maybe_function_ptr)) {
            set_runtime_error("Closure creation error, expected function");
            return InterpretResult::INTERPRET_RUNTIME_ERROR;
          }
          // TODO: Make naming better? function is in fact function_ptr.
          //       Same with closure.
          const function_ptr function = std::get<function_ptr>(maybe_function_ptr);
          const closure_ptr closure = heap->make<Closure>(function);
          stack.push_back(closure);
          for (uint8_t i = 0; i < function->upvalue_count; i++) {
            bool is_local = static_cast<bool>(READ_CODE());
            uint8_t index = READ_CODE();
            if (is_local) {
              closure->upvalues.push_back(add_or_get_upvalue(curr_frame->stack_offset + index));
              // Closing over local variable in enclosing (=currently executing) function.
            } else {
              closure->upvalues.push_back(curr_frame->closure.upvalues[index]);
              // Storing a pointer to an upvalue already captured by enclosing (=currently executing) function.
            }
          }
          break;
        }
        case OpCode::OP_GET_UPVALUE: {
          stack.push_back(*curr_frame->closure.upvalues[READ_CODE()]->value());
          break;
        }
        case OpCode::OP_SET_UPVALUE: {
          *curr_frame->closure.upvalues[READ_CODE()]->value() = stack.back();
          // TODO: Once GC is done -- verity this does not leak memory.

          // No .pop_back() as assignment is an expression and has to produce a
          // value.
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
          // No .pop_back() as assignment is an expression and has to produce a
          // value. In this case is the assigned value itself, eg. > print a = 8;
          // prints "8".
          break;
        }
        case OpCode::OP_GET_GLOBAL: {
          const Value maybe_var_name_ptr = curr_fun->chunk->constants[READ_CODE()]; 
          if (!std::holds_alternative<const_string_ptr>(maybe_var_name_ptr)) {
            set_runtime_error("Global variable name loading error, expected string");
            return InterpretResult::INTERPRET_RUNTIME_ERROR;
          }
          const auto var_name_ptr = std::get<const_string_ptr>(maybe_var_name_ptr);
          auto iter = globals.find(var_name_ptr);
          if (iter == globals.end()) {
            set_runtime_error("Undefined variable '" + *var_name_ptr + "'.");
            return InterpretResult::INTERPRET_RUNTIME_ERROR;
          }
          stack.push_back(iter->second);
          break;
        }
        case OpCode::OP_DEFINE_GLOBAL: {
          const Value maybe_var_name_ptr = curr_fun->chunk->constants[READ_CODE()];
          if (!std::holds_alternative<const_string_ptr>(maybe_var_name_ptr)) {
            set_runtime_error("Global variable name loading error, expected string");
            return InterpretResult::INTERPRET_RUNTIME_ERROR;
          }
          globals.insert_or_assign(std::get<const_string_ptr>(maybe_var_name_ptr),
                                  stack.back());
          stack.pop_back();
          break;
        }
        case OpCode::OP_SET_GLOBAL: {
          const Value maybe_var_name_ptr = curr_fun->chunk->constants[READ_CODE()];
          if (!std::holds_alternative<const_string_ptr>(maybe_var_name_ptr)) {
            set_runtime_error("Global variable name loading error, expected string");
            return InterpretResult::INTERPRET_RUNTIME_ERROR;
          }
          const auto var_name_ptr = std::get<const_string_ptr>(maybe_var_name_ptr);
          auto iter = globals.find(var_name_ptr);
          if (iter == globals.end()) {
            set_runtime_error("Undefined variable '" + *var_name_ptr + "'.");
            return InterpretResult::INTERPRET_RUNTIME_ERROR;
          }
          globals[var_name_ptr] = stack.back();
          // No .pop_back() as assignment is an expression and has to produce a
          // value. In this case is the assigned value itself, eg. > print a = 8;
          // prints "8".
          break;
        }
        case OpCode::OP_EQUAL: {
          // As with OP_RETURN - ensuring values remain alive to avoid GC collection.
          Value res {(*(stack.cend() - 2) == *(stack.cend() - 1))};
          stack.pop_back();
          stack.back() = res;
          break;
        }
        case OpCode::OP_GREATER:
          BINARY_OP(>);
          break;
        case OpCode::OP_LESS:
          BINARY_OP(<);
          break;
        case OpCode::OP_ADD: {
          // As with OP_RETURN - ensuring values remain alive to avoid GC collection.
          const Value rhs = *(stack.cend() - 1);
          const Value lhs = *(stack.cend() - 2);
          if (type_match<const_string_ptr>(lhs, rhs)) {
            Value res {pool->insert_or_get(*std::get<const_string_ptr>(lhs) + *std::get<const_string_ptr>(rhs))};
            stack.pop_back();
            stack.back() = res;
          } else if (type_match<double>(lhs, rhs)) {
            BINARY_OP(+);
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

  void VM::register_gc_callbacks() const {
    heap->register_root_marking_callback([this]() {
    #ifdef DEBUG_LOG_GC
    std::cout << "[VM] Marking stack" << std::endl;
    #endif
      const auto vmv {GCValueMarkingVisitor(heap)};
      for (const Value v : stack) {
        std::visit(vmv, v);
      }
    #ifdef DEBUG_LOG_GC
    std::cout << "[VM] Marking globals" << std::endl;
    #endif
      for (const auto& [k, v] : globals) {
        std::cout << "Marking " << to_string(k) << std::endl;
        heap->mark(k);
        std::visit(vmv, v);
      }
    #ifdef DEBUG_LOG_GC
    std::cout << "[VM] Marking call_frames" << std::endl;
    #endif
      for (const auto& cf : call_frames) {
        heap->mark(cf.closure.function);
        for (const auto uv : cf.closure.upvalues) {
          std::visit(vmv, *uv->value());
        }
        // TODO: Brittle, better to keep gc_ptrs in call_frames and just
        // mark entire closure.
      }
    #ifdef DEBUG_LOG_GC
    std::cout << "[VM] Marking open upvalues" << std::endl;
    #endif
      for(const auto uv : open_upvalues) {
        std::visit(vmv, *uv->value());
      }
      // TODO: In what cases anything that the closure owns is not reachable via stack?
      // Do closures need to be marked?
    #ifdef DEBUG_LOG_GC
      std::cout << "[VM] Done marking roots" << std::endl;
    #endif
    });
  }

  const upvalue_ptr VM::add_or_get_upvalue(size_t stack_index) {
    const auto iter = std::find_if(open_upvalues.crbegin(), open_upvalues.crend(), [&](const upvalue_ptr p){
      return p->stack_index() == stack_index;
    });
    // TODO: Early stop manual search? Or a way to efficiently do this in STL.

    if (iter != open_upvalues.crend()) {
      return *iter;
    } else {
      const auto insertion_iter = std::find_if(open_upvalues.begin(), open_upvalues.end(), [&](const upvalue_ptr p){
        return p->stack_index() > stack_index;
      });
      // TODO: Is this predicate correct? Print values to ensure correct order. Also consider unrolling into my own search.

      const auto ret {heap->make<RuntimeUpvalue>(stack, stack_index)};
      open_upvalues.insert(insertion_iter, ret);
      return ret;
    }
  }

  void VM::close_upvalues(size_t last) {
    while (!open_upvalues.empty() && open_upvalues.back()->stack_index() >= last) {
      open_upvalues.back()->close();
      open_upvalues.pop_back();
    }
  }

  bool VM::call(uint8_t arg_count) {
    if (call_frames.size() >= MAX_CALLSTACK_DEPTH) {
      set_runtime_error("Stackoverflow.");
      return false;
    }

    size_t callable_idx {stack.size() - 1 - arg_count};
    if (const auto* p = std::get_if<closure_ptr>(&stack[callable_idx])) {
      const Closure& c {**p};
      if (arg_count != c.function->arity) {
        set_runtime_error("Function " + *c.function->name + " expected " +
                          std::to_string(c.function->arity) + " parameters," + 
                          " but got " + std::to_string(arg_count) + ".");
        return false;
      }
      call_frames.emplace_back(c, 0, callable_idx);
      update_frame_pointers();
      return true;
    } else if (const auto* p = std::get_if<native_function_ptr>(&stack[callable_idx])) {
      Value ret {(*p)->func(arg_count, stack)};
      stack.erase(stack.end() - 1 - arg_count, stack.end());
      p = nullptr;
      // Removing args and native function from stack leaves p dangling, so doing it explicitly.
      stack.push_back(ret);
      return true;
    } else {
      set_runtime_error("Did not receive a callable.");
      return false;
    }
  }

  void VM::update_frame_pointers() {
    curr_frame = &call_frames.back();
    curr_fun = curr_frame->closure.function.get();
  }

  bool VM::is_falsey(Value val) const {
    if (std::holds_alternative<std::monostate>(val)) return true;
    if (std::holds_alternative<bool>(val) && !std::get<bool>(val)) return true;
    return false;
  }

  void VM::set_runtime_error(std::string err_msg) const {
    for (auto it = call_frames.crbegin(); it != call_frames.crend(); it++) {
      std::string call_site_line =
          std::to_string(it->closure.function->chunk->line_numbers[it->ip - 1]);
      std::cerr << "[line " + call_site_line + "] in " + *it->closure.function->name
                << std::endl;
      // TODO: Consider taking this stream as constructor param.
    }
    int line = curr_fun->chunk->line_numbers[curr_frame->ip];
    e_reporter.set_error("[Runtime error] [line " + std::to_string(line) +
                        "] while interpreting: " + err_msg);
  }

  void VM::trace_execution() const {
    log_output << "          ";
    if (stack.size() == 0) {
      log_output << "[]";
    } else {
      for (const auto& val : stack) {
        log_output << "[ " << to_string(val) << " ]";
      }
    }
    log_output << std::endl;
    log_output.flush();
    disassembler.disassemble_instruction(*curr_fun->chunk, curr_frame->ip - 1);
  }

  template <typename T>
  bool VM::type_match(Value lhs, Value rhs) const {
    return std::holds_alternative<T>(lhs) && std::holds_alternative<T>(rhs);
  }

}  // namespace cpplox