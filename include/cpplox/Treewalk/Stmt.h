#pragma once
#include <vector>

#include "cpplox/Treewalk/Expr.h"
#include "cpplox/Treewalk/StmtVisitorFwd.h"
#include "cpplox/Treewalk/Token.h"

namespace clox::Stmt {
using clox::Expr::ExprPtr;
using clox::Types::Token;

class Stmt;  // fwd declare
using StmtPtr = std::unique_ptr<Stmt>;

class Stmt {
 public:
  virtual void accept(StmtVisitor& visitor) const = 0;
  // TODO: figure out why specifiying other special functions breaks (copy
  // constructor etc).
  explicit Stmt() = default;
  virtual ~Stmt() = default;
};
class ExpressionStmt : public Stmt {
 public:
  explicit ExpressionStmt(ExprPtr expr) : expr(std::move(expr)) {}
  void accept(StmtVisitor& visitor) const override;

  ExprPtr expr;
};

class PrintStmt : public Stmt {
 public:
  explicit PrintStmt(ExprPtr expr) : expr(std::move(expr)) {}
  void accept(StmtVisitor& visitor) const override;

  ExprPtr expr;
};

class VarStmt : public Stmt {
 public:
  explicit VarStmt(Token name, ExprPtr initializer)
      : name(std::move(name)), initializer(std::move(initializer)) {}
  void accept(StmtVisitor& visitor) const override;

  Token name;
  ExprPtr initializer;
};

class BlockStmt : public Stmt {
 public:
  explicit BlockStmt(std::vector<StmtPtr> statements)
      : statements(std::move(statements)) {}
  void accept(StmtVisitor& visitor) const override;

  std::vector<StmtPtr> statements;
};

class IfStmt : public Stmt {
 public:
  explicit IfStmt(ExprPtr condition, StmtPtr then_branch, StmtPtr else_branch)
      : condition(std::move(condition)),
        then_branch(std::move(then_branch)),
        else_branch(std::move(else_branch)) {}
  void accept(StmtVisitor& visitor) const override;

  ExprPtr condition;
  StmtPtr then_branch;
  StmtPtr else_branch;
};

class WhileStmt : public Stmt {
 public:
  explicit WhileStmt(ExprPtr condition, StmtPtr body)
      : condition(std::move(condition)), body(std::move(body)) {}
  void accept(StmtVisitor& visitor) const override;

  ExprPtr condition;
  StmtPtr body;
};

class FunctionStmt : public Stmt {
 public:
  explicit FunctionStmt(Token name, std::vector<Token> params,
                        std::vector<StmtPtr> body)
      : name(std::move(name)),
        params(std::move(params)),
        body(std::move(body)) {}
  void accept(StmtVisitor& visitor) const override;

  Token name;
  std::vector<Token> params;
  std::vector<StmtPtr> body;
};

class ReturnStmt : public Stmt {
 public:
  explicit ReturnStmt(Token keyword, ExprPtr value)
      : keyword(std::move(keyword)), value(std::move(value)) {}
  void accept(StmtVisitor& visitor) const override;

  Token keyword;
  ExprPtr value;
};

}  // namespace clox::Stmt
