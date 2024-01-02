#include "cpplox/Treewalk/Runner.h"

#include <cassert>

#include <fstream>
#include <iostream>

#include "cpplox/Treewalk/Parser.h"
#include "cpplox/Treewalk/Scanner.h"
#include "cpplox/Treewalk/Token.h"

namespace clox {
void Runner::runFile(std::string path) {
  std::ifstream ifs(path);
  if (!ifs.good()) {
    throw std::runtime_error("File not found: " + path);
  }
  std::string source((std::istreambuf_iterator<char>(ifs)),
                     (std::istreambuf_iterator<char>()));
  run(source);
}

void Runner::runRepl() {
  std::string source{};
  while (true) {
    output << "> ";
    output.flush();
    getline(input, source);
    run(source);
    e_reporter.clear();
  }
}

void Runner::runCodeDebug() { runRepl(); }

void Runner::run(std::string source) {
  Scanner scanner{source, e_reporter};
  const std::vector<const Token> tokens{scanner.tokenize()};
  if (e_reporter.has_error()) {
    output << "[Scanning error] " << e_reporter.to_string();
    return;
  }

  Parser p{tokens, e_reporter};
  std::vector<StmtPtr> expr{p.parse()};
  if (e_reporter.has_error()) {
    output << "[Parsing error] " << e_reporter.to_string();
    return;
  }

  Resolver r{interpreter, e_reporter};
  r.resolve(expr);

  if (e_reporter.has_error()) {
    output << "[Resolving error] " << e_reporter.to_string();
    return;
  }

  interpreter.interpret(expr);
  if (e_reporter.has_error()) {
    output << "[Runtime error] " << e_reporter.to_string();
    return;
  }
}
}  // namespace clox