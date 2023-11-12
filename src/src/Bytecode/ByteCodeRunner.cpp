#include "cpplox/Bytecode/ByteCodeRunner.h"

#include <assert.h>

#include <fstream>
#include <iostream>

namespace clox {
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

  std::unique_ptr<Chunk> chunk =
      Compiler(tokens, disassembler, e_reporter).compile();
  if (e_reporter.has_error()) {
    output << e_reporter.to_string();
    output.flush();
    return;
  }
  assert(chunk);
  InterpretResult result =
      VM(output, disassembler, e_reporter, log_output).interpret(*chunk.get());
  if (e_reporter.has_error()) {
    output << e_reporter.to_string();
    return;
  }
  if (result != InterpretResult::INTERPRET_COMPILE_ERROR) {
  };  // Just to make compiler happy;
  return;
}
}  // namespace clox