#pragma once
#include <map>
#include <string>
#include <vector>

#include "cpplox/Treewalk/ErrorReporter.h"
#include "cpplox/Treewalk/Literal.h"
#include "cpplox/Treewalk/Token.h"

namespace clox {
using ErrorsAndDebug::ErrorReporter;
using Types::Literal;
using Types::Token;
using Types::TokenType;

/*
 * Based on text representation of source code, scanner produces a vector of
 * Tokens linearly representing the code. Ownership of the vector of tokens is
 * transferred to the caller.
 *
 * TODO: There is no reason for parser to be bound to a single source file =>
 * refactor to take source as param to tokenize.
 */
class Scanner {
 public:
  Scanner(std::string in_source, ErrorReporter &in_e_reporter)
      : source(in_source), e_reporter(in_e_reporter){};
  const std::vector<const Token> tokenize();

 private:
  void scan_token();
  void add_token(const TokenType &type);
  void add_token(const TokenType &type, const Literal &literal);
  char advance();
  bool can_read_more() const;
  bool match(const char expected);
  char peek() const;
  char peek_next() const;
  void consume_string();
  void consume_number();
  void consume_identifier();
  bool is_digit(char chr) const;
  bool is_alpha(char chr) const;
  bool is_alphanumeric(char chr) const;

  std::string source{};
  ErrorReporter &e_reporter;
  std::vector<const Token> tokens{};
  size_t current{0};
  size_t start{0};
  int line_number{1};
};

}  // namespace clox