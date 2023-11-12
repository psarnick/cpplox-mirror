#pragma once

#include <cassert>
#include <fstream>
#include <unordered_map>
#include <vector>

#include "cpplox/Bytecode/Chunk.h"
#include "cpplox/Bytecode/Compiler.h"
#include "cpplox/Bytecode/Debug.h"
#include "cpplox/Treewalk/ErrorReporter.h"

namespace vm {
using clox::ErrorsAndDebug::ErrorReporter;
using namespace debug;
using namespace chunk;

enum class InterpretResult {
  INTERPRET_OK = 0,
  INTERPRET_COMPILE_ERROR = 1,
  INTERPRET_RUNTIME_ERROR = 2
};
// TODO: ByteCodeRunner does scanning and compilation, so
// INTERPRET_COMPILE_ERROR is not applicable for now.

class VM {
 public:
  explicit VM(std::ostream& output, const Disassembler& disassembler,
              ErrorReporter& e_reporter, std::ofstream& log_output)
      : ip(0),
        output(output),
        disassembler{disassembler},
        log_output{log_output},
        e_reporter(e_reporter){};

  InterpretResult interpret(const Chunk& chunk);
  // interpret can only be called once, very meh but simpler for now.
  // TODO: Consider moving Chunks& arg to constructor (similar to Compiler.h) to
  // make contract explicit
 private:
  size_t ip;
  // Invariant: ip points to the instruction about to be executed.
  std::ostream& output;
  const Disassembler& disassembler;
  std::ofstream& log_output;
  ErrorReporter& e_reporter;
  std::unordered_map<std::string, Value> globals;
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
  // Unbounded stack
  // Needless copies of Value are made on stack and globals, maybe better to
  // pass Value by value (it's usually a small object) and just optimise
  // handling std::string case (off-hand pool?)
  bool already_called{false};

  InterpretResult run(const Chunk& chunk);
  bool is_falsey(const Value& val) const;
  bool values_equal(const Value& lhs, const Value& rhs) const;
  void set_runtime_error(std::string err_msg, int line) const;
  template <typename T>
  bool type_match(const Value& lhs, const Value& rhs);
  template <typename T>
  T add(const Value& lhs, const Value& rhs);
};

}  // namespace vm