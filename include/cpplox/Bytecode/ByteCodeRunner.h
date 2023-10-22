#pragma once
#include <fstream>
#include <memory>
#include <string>

#include "cpplox/Treewalk/ErrorReporter.h"
#include "cpplox/Treewalk/Scanner.h"
#include "cpplox/Treewalk/Token.h"
// TODO: Move above to common/per-functionality to break dependency.
#include "cpplox/Bytecode/Chunk.h"
#include "cpplox/Bytecode/Compiler.h"
#include "cpplox/Bytecode/VM.h"

namespace clox {
using ErrorsAndDebug::ErrorReporter;
using namespace vm;

class ByteCodeRunner {
 public:
  ByteCodeRunner(std::ostream& output = std::cout,
                 std::istream& input = std::cin,
                 const std::string log_fname = "compiler.log")
      : output(output),
        input(input),
        log_output(log_fname),
        e_reporter{},
        disassembler{log_output} {};
  void runFile(const std::string& path);
  void runRepl();

 private:
  std::ostream& output;
  std::istream& input;
  std::ofstream log_output;
  ErrorReporter e_reporter;
  const Disassembler disassembler;

  void run(const std::string& source);
};

}  // namespace clox