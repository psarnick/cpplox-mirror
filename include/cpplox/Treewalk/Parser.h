#pragma once
#include <memory>
#include <vector>

#include "cpplox/Treewalk/ErrorReporter.h"
#include "cpplox/Treewalk/Expr.h"
#include "cpplox/Treewalk/Stmt.h"
#include "cpplox/Treewalk/Token.h"

namespace clox {
using namespace clox::Types;
using namespace clox::Expr;
using namespace clox::Stmt;
using namespace clox::ErrorsAndDebug;

// TODO: Better to have this as exception that does not take c_str arg, or
// runtime_error?
class ParserException : public std::runtime_error {
 public:
  ParserException(const std::string& msg) : std::runtime_error(msg.c_str()) {}
};

/*
 * Based on the vector of tokens it takes, parser produces an AST represented by
 * vector of unique_ptrs. Parser does not take ownership of vector of tokens,
 * neither does it mutate any of the tokens. Ownership of AST is transferred to
 * caller.
 *
 * TODO: Explicit way to describe immutability of tokens => accept references to
 * const. Maybe even ptr to vec, not ref.
 * TODO: Important: ensure AST does not re-use memory from Tokens => value
 * instead of ref/ptr semantics.
 * TODO: An explicit way to describe the above idea of parser's statelessness is
 * to accept tokens to a parse method instead via constructor.
 */
class Parser {
 public:
  explicit Parser(const std::vector<const Token>& tokens,
                  ErrorReporter& e_reporter)
      : tokens(tokens), e_reporter{e_reporter} {}
  std::vector<StmtPtr> parse();

 private:
  StmtPtr declaration();
  StmtPtr function_declaration(std::string kind);
  StmtPtr var_declaration();
  StmtPtr statement();
  StmtPtr for_statement();
  StmtPtr if_statement();
  StmtPtr print_statement();
  StmtPtr while_statement();
  StmtPtr return_statement();
  StmtPtr expression_statement();
  std::vector<StmtPtr> block_statement();
  ExprPtr expression();
  ExprPtr assignment();
  ExprPtr logical_or();
  ExprPtr logical_and();
  ExprPtr equality();
  ExprPtr comparison();
  ExprPtr term();
  ExprPtr factor();
  ExprPtr unary();
  ExprPtr call();
  ExprPtr finish_call(ExprPtr callee);
  ExprPtr primary();

  void synchronize();
  bool match(const std::vector<TokenType>& tt);
  bool check_type(const TokenType& type) const;
  bool can_read_more() const;
  Token consume(const TokenType& type, const std::string& msg);
  Token peek() const;
  Token advance();
  Token previous() const;
  ParserException error(const Token& token, const std::string& msg,
                        bool override_msg = false);

  const std::vector<const Token> tokens;
  ErrorReporter& e_reporter;
  size_t current{0};
};

}  // namespace clox