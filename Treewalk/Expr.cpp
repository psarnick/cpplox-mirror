#include "cpplox/Treewalk/ExprVisitor.h"

namespace clox::Expr {
void BinaryExpr::accept(ExprVisitor& visitor) const {
  visitor.visitBinary(*this);
}
void GroupingExpr::accept(ExprVisitor& visitor) const {
  visitor.visitGrouping(*this);
}
void LiteralExpr::accept(ExprVisitor& visitor) const {
  visitor.visitLiteral(*this);
}
void UnaryExpr::accept(ExprVisitor& visitor) const {
  visitor.visitUnary(*this);
}
void VariableExpr::accept(ExprVisitor& visitor) const {
  visitor.visitVariable(*this);
}
void AssignExpr::accept(ExprVisitor& visitor) const {
  visitor.visitAssign(*this);
}
void LogicalExpr::accept(ExprVisitor& visitor) const {
  visitor.visitLogical(*this);
}
void CallExpr::accept(ExprVisitor& visitor) const { visitor.visitCall(*this); }
}  // namespace clox::Expr