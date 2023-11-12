#include "cpplox/Treewalk/Scanner.h"

#include <algorithm>
#include <iostream>
#include <string>

namespace clox {

using Types::Literal;
using Types::Token;
using Types::TokenType;

TokenType reserved_keyword_or_identifier_type(const std::string& str) {
  static const std::map<std::string, TokenType> lookUpTable{
      {"and", TokenType::AND},       {"class", TokenType::CLASS},
      {"else", TokenType::ELSE},     {"false", TokenType::FALSE},
      {"fun", TokenType::FUN},       {"for", TokenType::FOR},
      {"if", TokenType::IF},         {"nil", TokenType::NIL},
      {"or", TokenType::OR},         {"print", TokenType::PRINT},
      {"return", TokenType::RETURN}, {"super", TokenType::SUPER},
      {"this", TokenType::THIS},     {"true", TokenType::TRUE},
      {"var", TokenType::VAR},       {"while", TokenType::WHILE}};

  auto iter = lookUpTable.find(str);
  if (iter == lookUpTable.end()) {
    return TokenType::IDENTIFIER;
  }
  return iter->second;
}

const std::vector<const Token> Scanner::tokenize() {
  while (can_read_more()) {
    start = current;
    scan_token();
  }
  start = current;
  add_token(TokenType::LOX_EOF);
  return tokens;
}

void Scanner::scan_token() {
  const char chr = advance();
  switch (chr) {
    case '(':
      add_token(TokenType::LEFT_PAREN);
      break;
    case ')':
      add_token(TokenType::RIGHT_PAREN);
      break;
    case '{':
      add_token(TokenType::LEFT_BRACE);
      break;
    case '}':
      add_token(TokenType::RIGHT_BRACE);
      break;
    case ',':
      add_token(TokenType::COMMA);
      break;
    case '.':
      add_token(TokenType::DOT);
      break;
    case '-':
      add_token(TokenType::MINUS);
      break;
    case '+':
      add_token(TokenType::PLUS);
      break;
    case ';':
      add_token(TokenType::SEMICOLON);
      break;
    case '*':
      add_token(TokenType::STAR);
      break;
    case '!':
      add_token(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
      break;
    case '=':
      add_token(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
      break;
    case '<':
      add_token(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
      break;
    case '>':
      add_token(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
      break;
    case '/':
      if (match('/')) {
        while (can_read_more() && peek() != '\n') {
          advance();
        }
      } else {
        add_token(TokenType::SLASH);
      }
      break;
    case '"':
      consume_string();
      break;
    case '\n':
      line_number++;
      break;
    case ' ':
    case '\r':
    case '\t':
      break;
    default:
      if (is_digit(chr)) {
        consume_number();
      } else if (is_alpha(chr)) {
        consume_identifier();
      } else {
        e_reporter.set_error("[line " + std::to_string(line_number) +
                             "] error: unexpected character while scanning: '" +
                             std::string(1, chr) + "'");
      }
  }
}

void Scanner::add_token(const TokenType& type) {
  std::string lexeme{source.substr(start, current - start)};
  tokens.push_back(Token{type, lexeme, line_number});
}

void Scanner::add_token(const TokenType& type, const Literal& literal) {
  std::string lexeme{source.substr(start, current - start)};
  tokens.push_back(Token{type, lexeme, line_number, literal});
}

char Scanner::advance() { return source[current++]; }

bool Scanner::can_read_more() const { return current < source.length(); }

bool Scanner::match(const char expected) {
  if (!can_read_more() || source[current] != expected) {
    return false;
  }
  current++;
  return true;
}

char Scanner::peek() const { return can_read_more() ? source[current] : '\0'; }

char Scanner::peek_next() const {
  return current + 1 < source.length() ? source[current + 1] : '\0';
}

void Scanner::consume_string() {
  while (can_read_more() && peek() != '"') {
    if (peek() == '\n') {
      line_number++;
    }
    advance();
  }

  if (!can_read_more()) {
    e_reporter.set_error("[line " + std::to_string(line_number) +
                         "] error: unterminated string");
    return;
  }
  // closing "
  advance();
  // Remove escaped double quotes.
  std::string lit{source.substr(start, current - start)};
  lit.erase(std::remove(lit.begin(), lit.end(), '\"'), lit.end());

  add_token(TokenType::STRING, lit);
}

void Scanner::consume_number() {
  while (is_digit(peek())) {
    advance();
  }

  if (peek() == '.' && is_digit(peek_next())) {
    advance();
    while (is_digit(peek())) {
      advance();
    }
  }
  add_token(TokenType::NUMBER, stod(source.substr(start, current - start)));
}

void Scanner::consume_identifier() {
  while (is_alphanumeric(peek())) {
    advance();
  }

  add_token(clox::reserved_keyword_or_identifier_type(
      source.substr(start, current - start)));
}

bool Scanner::is_digit(char chr) const { return chr >= '0' && chr <= '9'; }

bool Scanner::is_alpha(char chr) const {
  return (chr >= 'a' && chr <= 'z') || (chr >= 'A' && chr <= 'Z') || chr == '_';
}
bool Scanner::is_alphanumeric(char chr) const {
  return is_alpha(chr) || is_digit(chr);
}

}  // namespace clox