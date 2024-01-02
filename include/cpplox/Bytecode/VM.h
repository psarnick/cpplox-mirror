#pragma once

#include <cassert>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <list>

#include "cpplox/Treewalk/ErrorReporter.h"
// TODO: Move above to common/per-functionality to break dependency.
#include "cpplox/Bytecode/Chunk.h"
#include "cpplox/Bytecode/Compiler.h"
#include "cpplox/Bytecode/Debug.h"
#include "cpplox/Bytecode/Value.h"
#include "cpplox/Bytecode/NativeFunctions.h"
#include "cpplox/Bytecode/StringPool.h"


namespace cpplox {

using clox::ErrorsAndDebug::ErrorReporter;


enum class InterpretResult {
  INTERPRET_OK = 0,
  INTERPRET_RUNTIME_ERROR = 1
};

class CallFrame {
  // Represents a single ongoing function call.
 public:
  explicit CallFrame(const Closure& closure, size_t ip, size_t stack_offset)
      : closure{closure}, ip{ip}, stack_offset{stack_offset} {};
  const Closure& closure;
  size_t ip{0};
  size_t stack_offset{0};
  // Local variable slots calculated by compiler are relative to function's
  // start (0th index is reserved, 1st local variable = 1, 2nd = 1, ...).
  // However, the VM has a single stack shared across many function invocations,
  // so to access the correct stack index, each CallFrame stores the first stack
  // slot that it can use.
};

class VM {
 public:
  explicit VM(std::ostream& output, const Disassembler& disassembler,
              ErrorReporter& e_reporter, gc_heap* const heap,
              StringPool* const pool, std::ofstream& log_output)
      : output(output),
        disassembler{disassembler},
        e_reporter(e_reporter),
        heap{heap},
        pool{pool},
        log_output{log_output} {
          register_gc_callbacks();
          globals.insert_or_assign(pool->insert_or_get("clock"), heap->make<NativeFn>(cpplox::clock));
        };

  InterpretResult interpret(function_ptr func);
  // interpret() can only be called once, very meh but simpler for now.
  // TODO: Consider param to constructor (similar to Compiler.h) to make
  // contract explicit.
  static const int MAX_CALLSTACK_DEPTH = 128;

 private:
  std::ostream& output;
  const Disassembler& disassembler;
  ErrorReporter& e_reporter;
  gc_heap* const heap;
  StringPool* const pool;
  std::ofstream& log_output;
  std::unordered_map<const_string_ptr, Value> globals;
  // Global variables are resolved dynamically (code referring to a global
  // variable before it's defined is valid, as long as this code is executed
  // after the corresponding definition - handy for recursive functions and
  // repl), but this means that globals cannot/are hard to be compiled
  // statically. Local variables, on the other hand, are always declared before
  // usage, get created in declaration statements and because statements don't
  // nest inside expressions there are never any temporary variables on the VM
  // stack when statement begins executing. As such, local variables can live
  // directly on stack if offsets from the top can be simulated at compile time
  // (which Compiler does) Further details:
  // https://craftinginterpreters.com/local-variables.html#representing-local-variables
  // TODO: What would it mean to compile a global variable?
  std::vector<Value> stack;
  // TODO: Bound the stack & throw error if exceeded.
  std::list<upvalue_ptr> open_upvalues{};
  // Captured values that are still in lexical scope (therefore on stack) and
  // can be directly referenced in other closures. TODO: adding classes
  // and fields can change the meaning of this list. Ordered by increasing
  // stack index (head is closest to stack bottom, tail to stack top).

  CallFrame* curr_frame{nullptr};
  const Function* curr_fun{nullptr};
  std::vector<CallFrame> call_frames;
  // TODO: Consider preallocating & reusing a pool of objects to make function
  // calls faster.
  bool already_called{false};

  InterpretResult run();
  void register_gc_callbacks() const;
  const upvalue_ptr add_or_get_upvalue(size_t stack_index);
  void close_upvalues(size_t last);
  void update_frame_pointers();
  bool call(uint8_t arg_count);
  bool is_falsey(Value val) const;
  void set_runtime_error(std::string err_msg) const;
  void trace_execution() const;
  template <typename T>
  bool type_match(Value lhs, Value rhs) const;
};

}  // namespace cpplox