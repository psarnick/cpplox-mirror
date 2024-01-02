#include "cpplox/Treewalk/StmtVisitor.h"

namespace clox::Stmt {
void ExpressionStmt::accept(StmtVisitor& visitor) const {
  visitor.visitExpression(*this);
}
void PrintStmt::accept(StmtVisitor& visitor) const {
  visitor.visitPrint(*this);
}
void VarStmt::accept(StmtVisitor& visitor) const { visitor.visitVar(*this); }
void BlockStmt::accept(StmtVisitor& visitor) const {
  visitor.visitBlock(*this);
}
void IfStmt::accept(StmtVisitor& visitor) const { visitor.visitIf(*this); }
void WhileStmt::accept(StmtVisitor& visitor) const {
  visitor.visitWhile(*this);
}
void FunctionStmt::accept(StmtVisitor& visitor) const {
  visitor.visitFunction(*this);
}
void ReturnStmt::accept(StmtVisitor& visitor) const {
  visitor.visitReturn(*this);
}
}  // namespace clox::Stmt