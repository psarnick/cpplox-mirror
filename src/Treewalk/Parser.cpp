#include "cpplox/Treewalk/Parser.h"

#include "cpplox/Treewalk/ExprFactory.h"
#include "cpplox/Treewalk/Stmt.h"

namespace clox {
using namespace clox::ExprFactory;

std::vector<StmtPtr> Parser::parse() {
  std::vector<StmtPtr> statements;
  while (can_read_more()) {
    try {
      // TODO: is this exception safe?
      statements.push_back(declaration());
    } catch (ParserException e) {
      e_reporter.set_error("[line " + std::to_string(peek().get_line()) +
                           "] error: bad syntax while parsing: " + e.what());
      synchronize();
    }
  }
  return statements;
}

StmtPtr Parser::declaration() {
  if (match({TokenType::FUN})) {
    return function_declaration("Function");
  }
  if (match({TokenType::VAR})) {
    return var_declaration();
  }
  return statement();
}

StmtPtr Parser::function_declaration(std::string kind) {
  Token name{consume(TokenType::IDENTIFIER, "Expected " + kind + "name.")};
  consume(TokenType::LEFT_PAREN, kind + " name must be followed by a '('");

  std::vector<Token> parameters{};
  if (!check_type(TokenType::RIGHT_PAREN)) {
    do {
      if (parameters.size() >= 255) {
        // No need to throw an error - that would attempt parser synchronisation
        // but current state in grammar is known. Just want to inform user and
        // continue.
        throw error(peek(), kind + " cannot have more that 255 parameters");
      }
      parameters.push_back(
          consume(TokenType::IDENTIFIER, "Expected parameter name."));
    } while (match({TokenType::COMMA}));
  }
  consume(TokenType::RIGHT_PAREN,
          kind + " parameter list must be followed by a ')'");
  consume(TokenType::LEFT_BRACE, kind + " body definition must start with '{'");
  return std::make_unique<FunctionStmt>(name, parameters, block_statement());
}

StmtPtr Parser::var_declaration() {
  Token name{consume(TokenType::IDENTIFIER,
                     "var keyword must be followed by variable name")};
  ExprPtr initializer{};
  if (match({TokenType::EQUAL})) {
    initializer = expression();
  }
  consume(TokenType::SEMICOLON, "variable definition must end with ';");
  return std::make_unique<VarStmt>(name, std::move(initializer));
}

StmtPtr Parser::statement() {
  if (match({TokenType::IF})) {
    return if_statement();
  }
  if (match({TokenType::FOR})) {
    return for_statement();
  }
  if (match({TokenType::PRINT})) {
    return print_statement();
  }
  if (match({TokenType::WHILE})) {
    return while_statement();
  }
  if (match({TokenType::RETURN})) {
    return return_statement();
  }
  if (match({TokenType::LEFT_BRACE})) {
    return std::make_unique<BlockStmt>(block_statement());
  }
  return expression_statement();
}

StmtPtr Parser::return_statement() {
  Token keyword{previous()};
  ExprPtr value{};
  if (!check_type(TokenType::SEMICOLON)) {
    value = expression();
  }
  consume(TokenType::SEMICOLON, "Return statement must terminate with a ';'");
  return std::make_unique<ReturnStmt>(keyword, std::move(value));
}

StmtPtr Parser::for_statement() {
  consume(TokenType::LEFT_PAREN, "'for' keyword must be followed by '(");

  StmtPtr initializer{};
  if (match({TokenType::VAR})) {
    initializer = var_declaration();
  } else if (match({TokenType::SEMICOLON})) {
    // pass - initializer already null
    // match has a side-effect of moving past ';'
  } else {
    initializer = expression_statement();
  }

  ExprPtr condition{};
  if (!check_type({TokenType::SEMICOLON})) {
    condition = expression();
  }

  consume(TokenType::SEMICOLON, "for loop condition must be followed by ';'");

  ExprPtr increment{};
  if (!check_type({TokenType::RIGHT_PAREN})) {
    increment = expression();
  }

  consume(TokenType::RIGHT_PAREN, "for loop clauses must be followed by ')");

  StmtPtr body{statement()};

  // desugar into a while loop
  if (increment) {
    StmtPtr expr_stmt{std::make_unique<ExpressionStmt>(std::move(increment))};
    std::vector<StmtPtr> statements;
    statements.push_back(std::move(body));
    statements.push_back(std::move(expr_stmt));
    body = std::make_unique<BlockStmt>(std::move(statements));
  }

  if (!condition) {
    condition = make_literal_expr(true);
  }

  body = std::make_unique<WhileStmt>(std::move(condition), std::move(body));

  if (initializer) {
    std::vector<StmtPtr> statements{};
    statements.push_back(std::move(initializer));
    statements.push_back(std::move(body));
    body = std::make_unique<BlockStmt>(std::move(statements));
  }
  return body;
}

StmtPtr Parser::if_statement() {
  consume(TokenType::LEFT_PAREN, "If keyword must be followed by '(");
  ExprPtr condition{expression()};
  consume(TokenType::RIGHT_PAREN,
          "Condition in if expression must be followed by ')");

  StmtPtr then_branch{statement()};

  StmtPtr else_branch{};
  if (match({TokenType::ELSE})) {
    else_branch = statement();
  }

  return std::make_unique<IfStmt>(std::move(condition), std::move(then_branch),
                                  std::move(else_branch));
}

StmtPtr Parser::print_statement() {
  ExprPtr val{expression()};
  consume(TokenType::SEMICOLON, "Print statements must end with ';'");
  return std::make_unique<PrintStmt>(std::move(val));
}

StmtPtr Parser::while_statement() {
  consume(TokenType::LEFT_PAREN, "'while' keyword must be followed by '(");
  ExprPtr expr{expression()};
  consume(TokenType::RIGHT_PAREN, "'while' condition must be followed by ')");

  return std::make_unique<WhileStmt>(std::move(expr), statement());
}

std::vector<StmtPtr> Parser::block_statement() {
  std::vector<StmtPtr> statements{};
  while (!check_type(TokenType::RIGHT_BRACE) && can_read_more()) {
    statements.push_back(declaration());
  }

  consume(TokenType::RIGHT_BRACE, "Block statements must end with '}'");
  return statements;
}

StmtPtr Parser::expression_statement() {
  ExprPtr val{expression()};
  consume(TokenType::SEMICOLON, "Expression statements must end with ';'");
  return std::make_unique<ExpressionStmt>(std::move(val));
}

ExprPtr Parser::expression() { return assignment(); }

ExprPtr Parser::assignment() {
  ExprPtr expr{logical_or()};
  if (match({TokenType::EQUAL})) {
    Token equals{previous()};
    ExprPtr value{assignment()};
    VariableExpr* ve{dynamic_cast<VariableExpr*>(expr.get())};
    if (ve) {
      return make_assignment_expr(*ve->name, std::move(value));
    }

    throw error(equals, "Invalid assignment target.");
  }
  return expr;
}

ExprPtr Parser::logical_or() {
  ExprPtr expr{logical_and()};
  while (match({TokenType::OR})) {
    // TODO: make_logical_expr(expr, previous(), and())
    // TODO: is function call order guaranteed to be L -> R?
    Token operator_{previous()};
    expr = make_logical_expr(std::move(expr), operator_, logical_and());
  }
  return expr;
}

ExprPtr Parser::logical_and() {
  ExprPtr expr{equality()};
  while (match({TokenType::AND})) {
    Token operator_{previous()};
    expr = make_logical_expr(std::move(expr), operator_, logical_and());
  }
  return expr;
}

ExprPtr Parser::equality() {
  ExprPtr expr{comparison()};

  while (match({TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL})) {
    Token operator_{previous()};
    expr = make_binary_expr(std::move(expr), operator_, comparison());
  }
  return expr;
}

ExprPtr Parser::comparison() {
  ExprPtr expr{term()};
  while (match({TokenType::GREATER, TokenType::GREATER_EQUAL, TokenType::LESS,
                TokenType::LESS_EQUAL})) {
    Token operator_{previous()};
    ExprPtr right{term()};
    expr = make_binary_expr(std::move(expr), operator_, std::move(right));
  }
  return expr;
}

ExprPtr Parser::term() {
  ExprPtr expr{factor()};
  while (match({TokenType::MINUS, TokenType::PLUS})) {
    Token operator_{previous()};
    ExprPtr right{factor()};
    expr = make_binary_expr(std::move(expr), operator_, std::move(right));
  }
  return expr;
}

ExprPtr Parser::factor() {
  ExprPtr expr{unary()};
  while (match({TokenType::SLASH, TokenType::STAR})) {
    Token operator_{previous()};
    ExprPtr right{unary()};
    expr = make_binary_expr(std::move(expr), operator_, std::move(right));
  }
  return expr;
}

ExprPtr Parser::unary() {
  if (match({TokenType::BANG, TokenType::MINUS})) {
    Token operator_{previous()};
    ExprPtr right{unary()};
    return make_unary_expr(operator_, std::move(right));
  }
  return call();
}

ExprPtr Parser::call() {
  ExprPtr expr{primary()};
  while (match({TokenType::LEFT_PAREN})) {
    expr = finish_call(std::move(expr));
  }
  return expr;
}

ExprPtr Parser::finish_call(ExprPtr callee) {
  std::vector<ExprPtr> arguments{};
  if (!check_type(TokenType::RIGHT_PAREN)) {
    do {
      if (arguments.size() >= 255) {
        // No need to throw an error - that would attempt parser synchronisation
        // but current state in grammar is known. Just want to inform user and
        // continue.
        error(peek(), "Function cannot take more than 255 arguments.", true);
      }
      arguments.push_back(expression());
    } while (match({TokenType::COMMA}));
  }
  Token paren{
      consume(TokenType::RIGHT_PAREN, "Argument list must end with ')")};
  return make_call_expr(std::move(callee), paren, std::move(arguments));
}

ExprPtr Parser::primary() {
  if (match({TokenType::FALSE})) {
    return make_literal_expr(false);
  }
  if (match({TokenType::TRUE})) {
    return make_literal_expr(true);
  }
  if (match({TokenType::NIL})) {
    return make_literal_expr(previous().get_literal());
  }
  if (match({TokenType::NUMBER, TokenType::STRING})) {
    return make_literal_expr(previous().get_literal());
  }
  if (match({TokenType::IDENTIFIER})) {
    return make_variable_expr(previous());
  }
  if (match({TokenType::LEFT_PAREN})) {
    ExprPtr expr{expression()};
    consume(TokenType::RIGHT_PAREN, "Expecting right paren )");
    return make_grouping_expr(std::move(expr));
  }

  throw error(peek(), "Expected expression.");
}

void Parser::synchronize() {
  advance();
  while (can_read_more()) {
    // stmts end with ; - prob synchronised once 1 past ;
    if (previous().get_type() == TokenType::SEMICOLON) {
      return;
    }

    // stms start with reserved keywords, prob synchronized when on reserved
    // keywrd
    switch (peek().get_type()) {
      case TokenType::CLASS:
      case TokenType::FUN:
      case TokenType::VAR:
      case TokenType::FOR:
      case TokenType::IF:
      case TokenType::WHILE:
      case TokenType::PRINT:
      case TokenType::RETURN:
        return;
      default:
        break;  // Noop, continue iterating until relevant found
    }
    advance();
  }
}

bool Parser::match(const std::vector<TokenType>& tt) {
  for (auto& t : tt) {
    if (check_type(t)) {
      advance();
      return true;
    }
  }
  return false;
}

bool Parser::check_type(const TokenType& type) const {
  return can_read_more() ? type == peek().get_type() : false;
}

bool Parser::can_read_more() const {
  return peek().get_type() != TokenType::LOX_EOF;
}

Token Parser::consume(const TokenType& t, const std::string& msg) {
  if (check_type(t)) {
    return advance();
  }

  throw error(peek(), msg);
}

Token Parser::peek() const { return tokens[current]; }

Token Parser::advance() {
  if (can_read_more()) {
    current++;
  }
  return previous();
}

Token Parser::previous() const { return tokens[current - 1]; }

ParserException Parser::error(const Token& token, const std::string& msg,
                              bool override_msg) {
  // TODO: refactor
  if (override_msg) {
    return ParserException(msg);
  } else {
    std::string full{
        "Got TokenType::" + Token::token_type_string(token.get_type()) +
        " with lexeme: '" + token.get_lexeme() + "'. " + msg};
    return ParserException(full);
  }
}

}  // namespace clox