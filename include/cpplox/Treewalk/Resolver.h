#pragma once
#include <map>
#include <vector>

#include "cpplox/Treewalk/ErrorReporter.h"
#include "cpplox/Treewalk/Expr.h"
#include "cpplox/Treewalk/ExprVisitor.h"
#include "cpplox/Treewalk/Interpreter.h"
#include "cpplox/Treewalk/Stmt.h"
#include "cpplox/Treewalk/StmtVisitor.h"
#include "cpplox/Treewalk/Token.h"

namespace clox {
using namespace ErrorsAndDebug;
using namespace clox::Expr;
using namespace clox::Stmt;
using namespace clox::Types;

enum class FunctionType { NONE, FUNCTION };

class Resolver : public ExprVisitor, public StmtVisitor {
 public:
  Resolver(Interpreter& intp, ErrorReporter& e_reporter)
      : intp(intp), e_reporter(e_reporter){};
  void resolve(const std::vector<StmtPtr>& stmts);

  void visitBinary(const BinaryExpr& binary_expr);
  void visitLiteral(const LiteralExpr& literal_expr);
  void visitGrouping(const GroupingExpr& grouping);
  void visitUnary(const UnaryExpr& unary);
  void visitVariable(const VariableExpr& var);
  void visitAssign(const AssignExpr& assign);
  void visitLogical(const LogicalExpr& logical);
  void visitCall(const CallExpr& call);

  void visitExpression(const ExpressionStmt& expr);
  void visitPrint(const PrintStmt& print);
  void visitVar(const VarStmt& var);
  void visitBlock(const BlockStmt& block);
  void visitIf(const IfStmt& if_);
  void visitWhile(const WhileStmt& while_);
  void visitFunction(const FunctionStmt& function);
  void visitReturn(const ReturnStmt& return_);

 private:
  void resolve(const ExprPtr& expr);
  void resolve(const StmtPtr& stms);
  // TODO: is there a way to simplify the types I use in this class? I have
  // ptrs, refs and interface refs
  void resolve_local_variable(const clox::Expr::Expr& expr, Token token);
  void resolve_function(const FunctionStmt& function, FunctionType type);

  void begin_scope();
  void end_scope();
  void declare(Token token);
  void define(Token token);

  Interpreter& intp;
  ErrorReporter& e_reporter;
  std::vector<std::map<std::string, bool>> scopes{};
  FunctionType current_function{FunctionType::NONE};
};

}  // namespace clox
