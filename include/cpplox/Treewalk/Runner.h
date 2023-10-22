#pragma once
#include <fstream>
#include <string>

#include "cpplox/Treewalk/ErrorReporter.h"
#include "cpplox/Treewalk/Interpreter.h"
#include "cpplox/Treewalk/Resolver.h"

namespace clox {
using ErrorsAndDebug::ErrorReporter;

class Runner {
 public:
  Runner(std::ostream& out = std::cout, std::istream& in = std::cin)
      : output(out), input(in), e_reporter{}, interpreter{e_reporter, out} {}
  void runFile(std::string path);
  void runRepl();
  void runCodeDebug();

 private:
  void run(std::string source);

  // TODO: how to make those two static/singletons?
  // TOOD: needed to preserve interpreter state in REPL
  std::ostream& output;
  std::istream& input;
  ErrorReporter e_reporter;
  Interpreter interpreter;
};

}  // namespace clox