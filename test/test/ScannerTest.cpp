#include "cpplox/Treewalk/Runner.h"
#include "cpplox/Treewalk/Scanner.h"
#include "cpplox/Treewalk/Token.h"
#include "gtest/gtest.h"

namespace clox {

using ErrorsAndDebug::LoxStatus;
using Types::literal_to_string;
using Types::Token;
using Types::TokenType;

TEST(ScannerTests, all_valid_tokens) {
  std::string source =
      "( ) { } , . - + ; / * ! != = == > >= < <= arg1 \"string\" 12 12.21 "
      "and class else false fun for if nil or print return super this true var "
      "while";
  ErrorReporter e_reporter{};
  clox::Scanner scanner{source, e_reporter};

  const std::vector<const Token> tokens = scanner.tokenize();

  std::vector<Types::TokenType> expected = {
      TokenType::LEFT_PAREN,    TokenType::RIGHT_PAREN,
      TokenType::LEFT_BRACE,    TokenType::RIGHT_BRACE,
      TokenType::COMMA,         TokenType::DOT,
      TokenType::MINUS,         TokenType::PLUS,
      TokenType::SEMICOLON,     TokenType::SLASH,
      TokenType::STAR,          TokenType::BANG,
      TokenType::BANG_EQUAL,    TokenType::EQUAL,
      TokenType::EQUAL_EQUAL,   TokenType::GREATER,
      TokenType::GREATER_EQUAL, TokenType::LESS,
      TokenType::LESS_EQUAL,    TokenType::IDENTIFIER,
      TokenType::STRING,        TokenType::NUMBER,
      TokenType::NUMBER,        TokenType::AND,
      TokenType::CLASS,         TokenType::ELSE,
      TokenType::FALSE,         TokenType::FUN,
      TokenType::FOR,           TokenType::IF,
      TokenType::NIL,           TokenType::OR,
      TokenType::PRINT,         TokenType::RETURN,
      TokenType::SUPER,         TokenType::THIS,
      TokenType::TRUE,          TokenType::VAR,
      TokenType::WHILE,         TokenType::LOX_EOF};

  std::vector<std::string> expected_lexems = {
      "(",          ")",     "{",     "}",      ",",
      ".",          "-",     "+",     ";",      "/",
      "*",          "!",     "!=",    "=",      "==",
      ">",          ">=",    "<",     "<=",     "arg1",
      "\"string\"", "12",    "12.21", "and",    "class",
      "else",       "false", "fun",   "for",    "if",
      "nil",        "or",    "print", "return", "super",
      "this",       "true",  "var",   "while",  "EOF_HAS_NO_LEXEME"};

  auto expected_iter = expected.cbegin();
  auto expected_lexeme_iter = expected_lexems.cbegin();
  for (auto& token : tokens) {
    EXPECT_EQ(token.get_type(), *expected_iter)
        << "[TYPE] Got: " << Token::token_type_string(token.get_type())
        << " expected " << Token::token_type_string(*expected_iter);
    expected_iter++;

    if (token.get_type() != TokenType::LOX_EOF) {
      EXPECT_EQ(token.get_lexeme(), *expected_lexeme_iter);
      expected_lexeme_iter++;
    }
  }
  ASSERT_EQ(e_reporter.has_error(), false);
}

TEST(ScannerTests, single_line_comment_with_source_below) {
  std::string source = "// foo(a | b); \n !=";
  ErrorReporter e_reporter{};
  clox::Scanner scanner{source, e_reporter};

  const std::vector<const Token> tokens = scanner.tokenize();

  ASSERT_EQ(tokens.size(), 2);
  ASSERT_EQ(tokens[0].get_type(), TokenType::BANG_EQUAL);
  ASSERT_EQ(tokens[0].get_lexeme(), "!=");
  ASSERT_EQ(tokens[1].get_type(), TokenType::LOX_EOF);
}

TEST(ScannerTests, strings_have_literals) {
  std::string source = "\"thisIsSomeString\"";
  ErrorReporter e_reporter{};
  clox::Scanner scanner{source, e_reporter};

  const std::vector<const Token> tokens = scanner.tokenize();
  ASSERT_EQ(tokens.size(), 2);
  ASSERT_EQ(std::holds_alternative<std::string>(tokens[0].get_literal()), true);
  ASSERT_EQ(literal_to_string(tokens[0].get_literal()), "thisIsSomeString");
  ASSERT_EQ(e_reporter.has_error(), false);
}

TEST(ScannerTests, tokens_have_corresponding_variant_types) {
  std::string source = "12 12.21 false for variable_name";
  ErrorReporter e_reporter{};
  clox::Scanner scanner{source, e_reporter};

  const std::vector<const Token> tokens = scanner.tokenize();
  ASSERT_EQ(tokens.size(), 6);

  ASSERT_EQ(std::holds_alternative<double>(tokens[0].get_literal()), true);
  ASSERT_EQ(std::get<double>(tokens[0].get_literal()), 12);
  ASSERT_EQ(tokens[0].get_type(), TokenType::NUMBER);

  ASSERT_EQ(std::holds_alternative<double>(tokens[1].get_literal()), true);
  ASSERT_EQ(std::get<double>(tokens[1].get_literal()), 12.21);
  ASSERT_EQ(tokens[1].get_type(), TokenType::NUMBER);

  // Keywords do not hold literals during scanning.
  // Parser creates values based on TokenType.
  ASSERT_EQ(std::holds_alternative<std::monostate>(tokens[2].get_literal()),
            true);
  ASSERT_EQ(std::get<std::monostate>(tokens[2].get_literal()),
            std::monostate());
  ASSERT_EQ(tokens[2].get_type(), TokenType::FALSE);

  ASSERT_EQ(std::holds_alternative<std::monostate>(tokens[3].get_literal()),
            true);
  ASSERT_EQ(std::get<std::monostate>(tokens[3].get_literal()),
            std::monostate());
  ASSERT_EQ(tokens[3].get_type(), TokenType::FOR);

  ASSERT_EQ(std::holds_alternative<std::monostate>(tokens[4].get_literal()),
            true);
  ASSERT_EQ(std::get<std::monostate>(tokens[4].get_literal()),
            std::monostate());
  ASSERT_EQ(tokens[4].get_type(), TokenType::IDENTIFIER);

  ASSERT_EQ(e_reporter.has_error(), false);
}

TEST(ScannerTests, unterminated_string) {
  std::string source = "\"";
  ErrorReporter e_reporter{};
  clox::Scanner scanner{source, e_reporter};

  const std::vector<const Token> tokens = scanner.tokenize();
  ASSERT_EQ(e_reporter.has_error(), true);
}

TEST(ScannerTests, number_and_literal_not_separated_by_space_are_two_tokens) {
  std::string source = "1.213abc";
  ErrorReporter e_reporter{};
  clox::Scanner scanner{source, e_reporter};

  const std::vector<const Token> tokens = scanner.tokenize();

  ASSERT_EQ(tokens.size(), 3);

  ASSERT_EQ(std::holds_alternative<double>(tokens[0].get_literal()), true);
  ASSERT_EQ(std::get<double>(tokens[0].get_literal()), 1.213);
  ASSERT_EQ(tokens[0].get_type(), TokenType::NUMBER);

  ASSERT_EQ(std::holds_alternative<std::monostate>(tokens[1].get_literal()),
            true);
  ASSERT_EQ(std::get<std::monostate>(tokens[1].get_literal()),
            std::monostate());
  ASSERT_EQ(tokens[1].get_type(), TokenType::IDENTIFIER);

  ASSERT_EQ(e_reporter.has_error(), false);
}

}  // namespace clox