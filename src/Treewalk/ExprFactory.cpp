#include "cpplox/Treewalk/ExprFactory.h"

#include <vector>

#include "cpplox/Treewalk/Literal.h"

namespace clox::ExprFactory {
ExprPtr make_binary_expr(ExprPtr left, const Token& operator_, ExprPtr right) {
  return std::make_unique<BinaryExpr>(
      std::move(left), std::make_unique<Token>(operator_), std::move(right));
}

ExprPtr make_grouping_expr(ExprPtr expr) {
  return std::make_unique<GroupingExpr>(std::move(expr));
}

ExprPtr make_literal_expr(const char* lit) {
  if (lit == nullptr) {
    throw std::runtime_error("Nullptr in make_literal_expr");
  }
  return std::make_unique<LiteralExpr>(
      std::make_unique<Literal>(std::string(lit)));
}

ExprPtr make_literal_expr(bool lit) {
  return std::make_unique<LiteralExpr>(std::make_unique<Literal>(lit));
}

ExprPtr make_literal_expr(const Literal& lit) {
  return std::make_unique<LiteralExpr>(std::make_unique<Literal>(lit));
}

ExprPtr make_unary_expr(const Token& operator_, ExprPtr right) {
  return std::make_unique<UnaryExpr>(std::make_unique<Token>(operator_),
                                     std::move(right));
}

ExprPtr make_variable_expr(const Token& name) {
  return std::make_unique<VariableExpr>(std::make_unique<Token>(name));
}

ExprPtr make_assignment_expr(const Token& name, ExprPtr expr) {
  return std::make_unique<AssignExpr>(std::make_unique<Token>(name),
                                      std::move(expr));
}

ExprPtr make_logical_expr(ExprPtr left, const Token& operator_, ExprPtr right) {
  return std::make_unique<LogicalExpr>(
      std::move(left), std::make_unique<Token>(operator_), std::move(right));
}

ExprPtr make_call_expr(ExprPtr callee, const Token& paren,
                       std::vector<ExprPtr> args) {
  return std::make_unique<CallExpr>(
      std::move(callee), std::make_unique<Token>(paren), std::move(args));
}
}  // namespace clox::ExprFactory