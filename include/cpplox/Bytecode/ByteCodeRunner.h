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
#include "cpplox/Bytecode/GC.h"
#include "cpplox/Bytecode/StringPool.h"

namespace cpplox {

  using clox::ErrorsAndDebug::ErrorReporter;
  // TODO: Clean up this legacy namespace.

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
    gc_heap heap {};
    StringPool pool {&heap};
    std::ostream& output;
    std::istream& input;
    std::ofstream log_output;
    ErrorReporter e_reporter;
    const Disassembler disassembler;

    void run(const std::string& source);
  };

}  // namespace cpplox