#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>

// #include "cpplox/Treewalk/AstGenerator.h"
#include "cpplox/Bytecode/ByteCodeRunner.h"
#include "cpplox/Bytecode/Debug.h"
#include "cpplox/Bytecode/VM.h"
#include "cpplox/Treewalk/Runner.h"

void adhoc() {
  // clox::ByteCodeRunner().runFile("C:\\Users\\psarn\\dev\\cpplox\\test\\block\\scope.lox");
  clox::Runner().runFile(
      "/Users/psarnick/dev/cpplox/test/function/too_many_parameters.lox");
  std::cout << "done" << std::endl;
}

void launch_treewalk(int argc, char* argv[]) {
  if (argc >= 2) {
    std::string arg{argv[1]};
    if (arg == "astgen") {
      // TODO: Align Stmt & Expr to either use TokenPtr or Token directly.
      /*
      generate_asts(
          "C:\\Users\\psarn\\dev\\cpplox\\src\\Expr",
          "Expr",
          {
              {"Binary", "ExprPtr left", "TokenPtr operator_", "ExprPtr right"},
              {"Grouping", "ExprPtr expression"},
              {"Literal", "LiteralPtr literal"},
              {"Unary", "TokenPtr operator_", "ExprPtr right"},
              {"Variable", "TokenPtr name"},
              {"Assign", "TokenPtr name", "ExprPtr value"},
              {"Logical", "ExprPtr left", "TokenPtr operator_", "ExprPtr
      right"},
              {"Call", "ExprPtr callee", "TokenPtr paren", "std::vector<ExprPtr>
      arguments"}
          }
      );

      generate_asts(
          "C:\\Users\\psarn\\dev\\cpplox\\src\\Stmt",
          "Stmt",
          {
              {"Expression", "ExprPtr expr"},
              {"Print", "ExprPtr expr"},
              {"Var", "Token name", "ExprPtr initializer"},
              {"Block", "std::vector<StmtPtr> statements"},
              {"If", "ExprPtr condition", "StmtPtr then_branch", "StmtPtr
      else_branch"},
              {"While", "ExprPtr condition", "StmtPtr body"},
              {"Function", "Token name", "std::vector<Token> params",
      "std::vector<StmtPtr> body"},
              {"Return", "Token keyword", "ExprPtr value"}
          }
      );
      */
    } else if (arg == "adhoc-test") {
      adhoc();
    } else if (arg == "adhoc") {
      std::string path{std::string(argv[2])};
      std::cout << "Output for start: " << path << std::endl;
      clox::Runner().runFile(path);
      std::cout << "Output for done: " << path << std::endl;
    } else {
      clox::Runner().runFile(argv[1]);
    }
  } else {
    clox::Runner().runRepl();
  }
}

void launch_bytecode(int argc, char* argv[]) {
  std::cout << "Running bytecode" << std::endl;
  if (argc == 2) {
    std::string arg{argv[1]};
    clox::ByteCodeRunner().runFile(argv[1]);
  } else if (argc == 1) {
    clox::ByteCodeRunner().runRepl();
  } else if (argc == 3) {
    // Debugger runs with 3 args
    clox::ByteCodeRunner().runFile(
        "/Users/psarnick/dev/cpplox/test/for/syntax.lox");
    clox::ByteCodeRunner().runRepl();
  } else {
    throw std::logic_error("Unsupported arguments " + std::to_string(argc));
  }
}

int main(int argc, char* argv[]) {
  // launch_treewalk(argc, argv);
  launch_bytecode(argc, argv);
}
