#include <iostream>
#include <memory>
#include <vector>

#include "cpplox/Treewalk/AstPrinter.h"
#include "cpplox/Treewalk/ErrorReporter.h"
#include "cpplox/Treewalk/Parser.h"
#include "gtest/gtest.h"

namespace clox {
/// TODO: Tokens vector receives manually created tokens, which may differ
///  from what Scanner would create Tokens. Consider using the same
///  method/testing that scanner would produce the same vector.

using clox::AstPrinter;
using clox::Types::Literal;
using clox::Types::Token;
using clox::Types::TokenType;
using ErrorsAndDebug::ErrorReporter;

TEST(AstParser, test_compound_expression) {
  //((-1))-(1*2)
  std::vector<const Token> tokens{};
  tokens.push_back({TokenType::LEFT_PAREN, "(", 0});
  tokens.push_back({TokenType::LEFT_PAREN, "(", 0});
  tokens.push_back({TokenType::MINUS, "-", 0});
  tokens.push_back({TokenType::NUMBER, "1", 0, 1.0});
  tokens.push_back({TokenType::RIGHT_PAREN, ")", 0});
  tokens.push_back({TokenType::RIGHT_PAREN, ")", 0});
  tokens.push_back({TokenType::MINUS, "-", 0});
  tokens.push_back({TokenType::LEFT_PAREN, "(", 0});
  tokens.push_back({TokenType::NUMBER, "1", 0, 1.0});
  tokens.push_back({TokenType::STAR, "*", 0});
  tokens.push_back({TokenType::NUMBER, "2", 0, 2.0});
  tokens.push_back({TokenType::RIGHT_PAREN, ")", 0});
  tokens.push_back({TokenType::SEMICOLON, ";", 0});
  tokens.push_back({TokenType::LOX_EOF, "EOF", 0});

  ErrorReporter e_reporter{};
  Parser p{tokens, e_reporter};

  std::vector<StmtPtr> expr{p.parse()};
  AstPrinter printer{};
  expr[0]->accept(printer);
  std::string result{printer.result()};
  ASSERT_EQ(result, "(expr_stmt (- (group (group (- 1))) (group (* 1 2))))");
};

TEST(AstParser, parper_continues_on_error_and_sets_err_message) {
  std::vector<const Token> tokens{};
  tokens.push_back({TokenType::NUMBER, "1", 0, 1.0});
  tokens.push_back({TokenType::BANG_EQUAL, "!=", 0});
  tokens.push_back({TokenType::RIGHT_PAREN, ")", 0});  // unexpected
  tokens.push_back({TokenType::NUMBER, "2", 0, 2.0});
  tokens.push_back({TokenType::SEMICOLON, ";", 0});
  tokens.push_back({TokenType::PRINT, "print", 1});
  tokens.push_back({TokenType::STRING, "a", 1, {"a"}});
  tokens.push_back({TokenType::LOX_EOF, "EOF", 1});
  // missing semicolon

  ErrorReporter e{};
  Parser p{tokens, e};
  p.parse();
  ASSERT_EQ(e.has_error(), true);
  std::string msg{
      "[line 0] error: bad syntax while parsing: Got TokenType::RIGHT_PAREN "
      "with lexeme: ')'. Expected expression.\n[line 1] error: bad syntax "
      "while parsing: Got TokenType::EOF with lexeme: 'EOF'. Print statements "
      "must end with ';'\n"};
  ASSERT_EQ(e.to_string(), msg);
};
}  // namespace clox