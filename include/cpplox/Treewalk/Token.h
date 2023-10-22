#pragma once
#include <string>

#include "cpplox/Treewalk/Literal.h"

namespace clox::Types {

enum class TokenType {
  // Single-character tokens.
  LEFT_PAREN,
  RIGHT_PAREN,
  LEFT_BRACE,
  RIGHT_BRACE,
  COMMA,
  DOT,
  MINUS,
  PLUS,
  SEMICOLON,
  SLASH,
  STAR,

  // One or two character tokens.
  BANG,
  BANG_EQUAL,
  EQUAL,
  EQUAL_EQUAL,
  GREATER,
  GREATER_EQUAL,
  LESS,
  LESS_EQUAL,

  // Literals.
  IDENTIFIER,
  STRING,
  NUMBER,

  // Keywords.
  AND,
  CLASS,
  ELSE,
  FALSE,
  FUN,
  FOR,
  IF,
  NIL,
  OR,
  PRINT,
  RETURN,
  SUPER,
  THIS,
  TRUE,
  VAR,
  WHILE,

  LOX_EOF
};

class Token {
 public:
  // TODO: Should Optional litral be initialized with lexeme when no literal
  // provided?
  Token(TokenType in_type, std::string in_lexeme, int in_line,
        Literal in_literal = std::monostate())
      : type(in_type), lexeme(in_lexeme), line(in_line), literal(in_literal) {}

  static const std::string& token_type_string(const TokenType val);
  std::string get_lexeme() const;
  TokenType get_type() const;
  int get_line() const;
  Literal get_literal() const;
  std::string to_string() const;

 private:
  const TokenType type;
  const std::string lexeme;
  const int line;
  const Literal literal;
};

}  // namespace clox::Types