#include <cassert>
#include <fstream>
#include <iostream>

#include "cpplox/Bytecode/ByteCodeRunner.h"
#include "cpplox/Bytecode/Value.h"

namespace cpplox {

  void ByteCodeRunner::runFile(const std::string& path) {
    std::ifstream ifs(path);
    std::string source((std::istreambuf_iterator<char>(ifs)),
                      (std::istreambuf_iterator<char>()));
    run(source);
  }

  void ByteCodeRunner::runRepl() {
    std::string line{};
    std::string source{};
    while (true) {
      output << "> ";
      output.flush();
      getline(input, line);
      source = source + "\n" + line;
      run(source);
    }
  }

  void ByteCodeRunner::run(const std::string& source) {
    // TODO: Both the Compiler and VM register root marking callbacks with heap.
    // However, heap outlives both of those leaving dangling callbacks after this
    // block is done. Either unregister those callbacks upon destruction or make
    // lifetimes identical + add comment.

    e_reporter.clear();
    log_output << " ByteCodeRunner state cleaned." << std::endl;
    log_output << "=== source ===" << std::endl;
    log_output << source << std::endl;
    log_output << "==/ source /==" << std::endl;

    const std::vector<const Token>& tokens =
        clox::Scanner{source, e_reporter}.tokenize();
    if (e_reporter.has_error()) {
      output << "[Scanning error] " << e_reporter.to_string();
      // TODO: Modify Scanner to include it's own [tag] & adjust Runner.cpp
      return;
    }

    std::optional<function_ptr> maybe_function =
        Compiler(tokens, disassembler, e_reporter, &heap, &pool).compile();
    if (e_reporter.has_error()) {
      output << e_reporter.to_string();
      output.flush();
      return;
    }
    assert(maybe_function.has_value());
    InterpretResult result = VM(output, disassembler, e_reporter, &heap, &pool, log_output)
                                .interpret(maybe_function.value());
    if (e_reporter.has_error()) {
      output << e_reporter.to_string();
      output.flush();
      return;
    }
    if (result != InterpretResult::INTERPRET_RUNTIME_ERROR) {
    };  // Just to make compiler happy as all errors are printed above.
    return;
  }
}  // namespace cpplox