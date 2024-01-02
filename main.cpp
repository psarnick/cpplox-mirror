#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <unordered_map>

// #include "cpplox/Treewalk/AstGenerator.h"
#include "cpplox/Bytecode/ByteCodeRunner.h"
#include "cpplox/Bytecode/Debug.h"
#include "cpplox/Bytecode/VM.h"
#include "cpplox/Treewalk/Runner.h"
#include "cpplox/Bytecode/GC.h"


void launch_bytecode(int argc, char* argv[]) {
  std::cout << "Running bytecode" << std::endl;
  if (argc == 2) {
    std::string arg{argv[1]};
    cpplox::ByteCodeRunner().runFile(argv[1]);
  } else if (argc == 1) {
    cpplox::ByteCodeRunner().runRepl();
  } else if (argc == 3) {
    // Debugger runs with 3 args
    cpplox::ByteCodeRunner().runFile(
    //"/Users/psarnick/dev/cpplox/test/closure/assign_to_closure.lox");
    "/Users/psarnick/dev/cpplox/build/sharing_captured_value.lox");
  } else {
    throw std::logic_error("Unsupported arguments " + std::to_string(argc));
  }
}

int main(int argc, char* argv[]) {
  launch_bytecode(argc, argv);
  return 0;
}
