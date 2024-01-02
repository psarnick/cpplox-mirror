#include "cpplox/Treewalk/Token.h"

#include <iostream>
#include <map>

namespace clox::Types {

const std::string& Token::token_type_string(const TokenType val) {
  static const std::map<TokenType, std::string> lookUpTable{
      {TokenType::LEFT_PAREN, "LEFT_PAREN"},
      {TokenType::RIGHT_PAREN, "RIGHT_PAREN"},
      {TokenType::LEFT_BRACE, "LEFT_BRACE"},
      {TokenType::RIGHT_BRACE, "RIGHT_BRACE"},
      {TokenType::COMMA, "COMMA"},
      {TokenType::DOT, "DOT"},
      {TokenType::SEMICOLON, "SEMICOLON"},
      {TokenType::SLASH, "SLASH"},
      {TokenType::STAR, "STAR"},
      {TokenType::BANG, "BANG"},
      {TokenType::BANG_EQUAL, "BANG_EQUAL"},
      {TokenType::EQUAL, "EQUAL"},
      {TokenType::EQUAL_EQUAL, "EQUAL_EQUAL"},
      {TokenType::GREATER, "GREATER"},
      {TokenType::GREATER_EQUAL, "GREATER_EQUAL"},
      {TokenType::LESS, "LESS"},
      {TokenType::LESS_EQUAL, "LESS_EQUAL"},
      {TokenType::MINUS, "MINUS"},
      {TokenType::PLUS, "PLUS"},
      {TokenType::IDENTIFIER, "IDENTIFIER"},
      {TokenType::STRING, "STRING"},
      {TokenType::NUMBER, "NUMBER"},
      {TokenType::AND, "AND"},
      {TokenType::CLASS, "CLASS"},
      {TokenType::ELSE, "ELSE"},
      {TokenType::FALSE, "FALSE"},
      {TokenType::FUN, "FUN"},
      {TokenType::FOR, "FOR"},
      {TokenType::IF, "IF"},
      {TokenType::NIL, "NIL"},
      {TokenType::OR, "OR"},
      {TokenType::PRINT, "PRINT"},
      {TokenType::RETURN, "RETURN"},
      {TokenType::SUPER, "SUPER"},
      {TokenType::THIS, "THIS"},
      {TokenType::TRUE, "TRUE"},
      {TokenType::VAR, "VAR"},
      {TokenType::WHILE, "WHILE"},
      {TokenType::LOX_EOF, "EOF"}};

  return lookUpTable.find(val)->second;
}

std::string Token::get_lexeme() const { return lexeme; }

TokenType Token::get_type() const { return type; }

int Token::get_line() const { return line; }

Literal Token::get_literal() const { return literal; }

std::string Token::to_string() const {
  return "{token_type: " + token_type_string(type) + "}, {lexeme: " + lexeme +
         "}, {literal: " + literal_to_string(literal) + "}";
}

}  // namespace clox::Types