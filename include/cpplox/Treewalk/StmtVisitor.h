#pragma once
#include "Stmt.h"

namespace clox::Stmt {
class StmtVisitor {
 public:
  virtual void visitExpression(const ExpressionStmt& expression) = 0;
  virtual void visitPrint(const PrintStmt& print) = 0;
  virtual void visitVar(const VarStmt& var) = 0;
  virtual void visitBlock(const BlockStmt& block) = 0;
  virtual void visitIf(const IfStmt& if_) = 0;
  virtual void visitWhile(const WhileStmt& while_) = 0;
  virtual void visitFunction(const FunctionStmt& function) = 0;
  virtual void visitReturn(const ReturnStmt& return_) = 0;

  // base class boilderplate
  explicit StmtVisitor() = default;
  virtual ~StmtVisitor() = default;
  StmtVisitor(const StmtVisitor&) = delete;
  StmtVisitor& operator=(const StmtVisitor&) = delete;
  StmtVisitor(StmtVisitor&&) = delete;
  StmtVisitor& operator=(StmtVisitor&&) = delete;
};
}  // namespace clox::Stmt
