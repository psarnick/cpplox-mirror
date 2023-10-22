#pragma once
#include <string>

#include "cpplox/Treewalk/Expr.h"
#include "cpplox/Treewalk/ExprVisitor.h"
#include "cpplox/Treewalk/Stmt.h"
#include "cpplox/Treewalk/StmtVisitor.h"

// TODO: Move to astprinter.cpp + astgen candidate
namespace clox {
using namespace clox::Expr;
using namespace clox::Stmt;

class AstPrinter : public ExprVisitor, public StmtVisitor {
  // TODO: Try variadic function template to paranthesize
 public:
  void visitBinary(const BinaryExpr& binary_expr) {
    result_ += "(";
    result_ += binary_expr.operator_->get_lexeme();
    result_ += " ";
    binary_expr.left->accept(*this);
    result_ += " ";
    binary_expr.right->accept(*this);
    result_ += ")";
  }

  void visitLiteral(const LiteralExpr& literal_expr) {
    result_ += clox::Types::literal_to_string(*literal_expr.literal);
  }

  void visitGrouping(const GroupingExpr& grouping) {
    result_ += "(group ";
    grouping.expression->accept(*this);
    result_ += ")";
  }
  void visitUnary(const UnaryExpr& unary) {
    result_ += "(" + unary.operator_->get_lexeme() + " ";
    unary.right->accept(*this);
    result_ += ")";
  }
  void visitVariable(const VariableExpr& var) {
    result_ += var.name->get_lexeme();
  }
  void visitAssign(const AssignExpr& assign) { result_ += "assign"; }
  void visitLogical(const LogicalExpr& logical) { result_ += "w/e"; }
  void visitCall(const CallExpr& logical) { result_ += "w/e"; }
  void visitPrint(const PrintStmt& print) {
    result_ += "(print ";
    print.expr->accept(*this);
    result_ += ")";
  }
  void visitExpression(const ExpressionStmt& expr) {
    result_ += "(expr_stmt ";
    expr.expr->accept(*this);
    result_ += ")";
  }
  void visitVar(const VarStmt& var) {
    result_ += "(var_stmt " + var.name.get_lexeme();
    if (var.initializer) {
      result_ += " = ";
      var.initializer->accept(*this);
      result_ += ";";
    }
    result_ += ")";
  }

  void visitBlock(const BlockStmt& block) {
    result_ += "{";
    for (const auto& s : block.statements) {
      s->accept(*this);
    }
    result_ += "};";
  }

  void visitIf(const IfStmt& if_) {
    result_ += "(if (";
    if_.condition->accept(*this);
    result_ += ")";
    if_.then_branch->accept(*this);
    if (if_.else_branch) {
      if_.else_branch->accept(*this);
    }
    result_ += ")";
  }
  void visitWhile(const WhileStmt& while_) { result_ += "w/e"; }
  void visitFunction(const FunctionStmt& while_) { result_ += "w/e"; }
  void visitReturn(const ReturnStmt& while_) { result_ += "w/e"; }

  const std::string& result() const { return result_; }

 private:
  std::string result_;
};
}  // namespace clox
