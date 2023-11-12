#include "cpplox/Treewalk/Resolver.h"

namespace clox {
void Resolver::resolve(const std::vector<StmtPtr>& stmts) {
  for (auto const& stmt : stmts) resolve(stmt);
}

void Resolver::resolve(const StmtPtr& stmt) { stmt->accept(*this); }

void Resolver::resolve(const ExprPtr& expr) { expr->accept(*this); }

void Resolver::resolve_local_variable(const clox::Expr::Expr& expr,
                                      Token token) {
  for (int i = scopes.size() - 1; i >= 0; --i) {
    if (scopes.at(i).find(token.get_lexeme()) != scopes.at(i).end()) {
      intp.resolve(expr, scopes.size() - 1 - i);
    }
  }
}

void Resolver::resolve_function(const FunctionStmt& function,
                                FunctionType type) {
  FunctionType enclosing = current_function;
  current_function = type;
  begin_scope();
  for (auto const& token : function.params) {
    declare(token);
    define(token);
  }
  resolve(function.body);
  end_scope();
  current_function = enclosing;
}

void Resolver::declare(Token token) {
  if (scopes.empty()) return;
  scopes.back().insert({token.get_lexeme(), false});
}

void Resolver::define(Token token) {
  if (scopes.empty()) return;
  scopes.back()[token.get_lexeme()] = true;
}

void Resolver::begin_scope() { scopes.push_back({}); }

void Resolver::end_scope() { scopes.pop_back(); }

void Resolver::visitBinary(const BinaryExpr& binary_expr) {
  resolve(binary_expr.left);
  resolve(binary_expr.right);
}

void Resolver::visitLiteral(const LiteralExpr& literal_expr) { return; }

void Resolver::visitGrouping(const GroupingExpr& grouping) {
  resolve(grouping.expression);
}

void Resolver::visitUnary(const UnaryExpr& unary) { resolve(unary.right); }

void Resolver::visitVariable(const VariableExpr& var) {
  bool var_not_ready =
      !scopes.empty() &&
      scopes.back().find(var.name->get_lexeme()) != scopes.back().end() &&
      scopes.back().find(var.name->get_lexeme())->second == false;
  if (var_not_ready) {
    e_reporter.set_error(
        "[line " + std::to_string(var.name->get_line()) +
        "] while resolving: Can't read local variable in its own initializer.");
  }
  resolve_local_variable(var, *var.name);
}

void Resolver::visitAssign(const AssignExpr& assign) {
  resolve(assign.value);
  resolve_local_variable(assign, *assign.name);
}

void Resolver::visitLogical(const LogicalExpr& logical) {
  resolve(logical.left);
  resolve(logical.right);
}

void Resolver::visitCall(const CallExpr& call) {
  resolve(call.callee);
  for (auto const& arg : call.arguments) {
    resolve(arg);
  }
}
void Resolver::visitExpression(const ExpressionStmt& expr) {
  resolve(expr.expr);
}

void Resolver::visitPrint(const PrintStmt& print) { resolve(print.expr); }

void Resolver::visitVar(const VarStmt& var) {
  declare(var.name);
  if (var.initializer) {
    resolve(var.initializer);
  }
  define(var.name);
}

void Resolver::visitBlock(const BlockStmt& block) {
  begin_scope();
  resolve(block.statements);
  end_scope();
}

void Resolver::visitIf(const IfStmt& if_) {
  resolve(if_.condition);
  resolve(if_.then_branch);
  if (if_.else_branch) resolve(if_.else_branch);
}

void Resolver::visitWhile(const WhileStmt& while_) {
  resolve(while_.condition);
  resolve(while_.body);
}

void Resolver::visitFunction(const FunctionStmt& function) {
  declare(function.name);
  define(function.name);
  resolve_function(function, FunctionType::FUNCTION);
}

void Resolver::visitReturn(const ReturnStmt& return_) {
  if (current_function == FunctionType::NONE) {
    e_reporter.set_error(
        "[line " + std::to_string(return_.keyword.get_line()) +
        "] while resolving: Can't return from top-level code.");
  }
  if (return_.value) resolve(return_.value);
}
}  // namespace clox