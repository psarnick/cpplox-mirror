#pragma once
#include "Expr.h"

namespace clox::Expr {
class ExprVisitor {
 public:
  virtual void visitBinary(const BinaryExpr& binary) = 0;
  virtual void visitGrouping(const GroupingExpr& grouping) = 0;
  virtual void visitLiteral(const LiteralExpr& literal) = 0;
  virtual void visitUnary(const UnaryExpr& unary) = 0;
  virtual void visitVariable(const VariableExpr& variable) = 0;
  virtual void visitAssign(const AssignExpr& assign) = 0;
  virtual void visitLogical(const LogicalExpr& logical) = 0;
  virtual void visitCall(const CallExpr& call) = 0;

  // base class boilderplate
  explicit ExprVisitor() = default;
  virtual ~ExprVisitor() = default;
  ExprVisitor(const ExprVisitor&) = delete;
  ExprVisitor& operator=(const ExprVisitor&) = delete;
  ExprVisitor(ExprVisitor&&) = delete;
  ExprVisitor& operator=(ExprVisitor&&) = delete;
};
}  // namespace clox::Expr
