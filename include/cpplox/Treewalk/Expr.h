#pragma once
#include <vector>

#include "cpplox/Treewalk/ExprVisitorFwd.h"
#include "cpplox/Treewalk/Token.h"

namespace clox::Expr {
using clox::Types::Literal;
using clox::Types::Token;

class Expr;  // fwd declare
using ExprPtr = std::unique_ptr<Expr>;
using TokenPtr = std::unique_ptr<Token>;
using LiteralPtr = std::unique_ptr<Literal>;

class Expr {
 public:
  virtual void accept(ExprVisitor& visitor) const = 0;
  // TODO: figure out why specifiying other special functions breaks (copy
  // constructor etc).
  explicit Expr() = default;
  virtual ~Expr() = default;
};
class BinaryExpr : public Expr {
 public:
  explicit BinaryExpr(ExprPtr left, TokenPtr operator_, ExprPtr right)
      : left(std::move(left)),
        operator_(std::move(operator_)),
        right(std::move(right)) {}
  void accept(ExprVisitor& visitor) const override;

  ExprPtr left;
  TokenPtr operator_;
  ExprPtr right;
};

class GroupingExpr : public Expr {
 public:
  explicit GroupingExpr(ExprPtr expression)
      : expression(std::move(expression)) {}
  void accept(ExprVisitor& visitor) const override;

  ExprPtr expression;
};

class LiteralExpr : public Expr {
 public:
  explicit LiteralExpr(LiteralPtr literal) : literal(std::move(literal)) {}
  // TODO: clox::Ast::LiteralExpr lit_expr{ "foo" }; works
  // but LiteralExpr takes Literal argument -> what conversion happens here?
  // especially since replacing const Literal& ctor with string breaks the above
  void accept(ExprVisitor& visitor) const override;

  LiteralPtr literal;
};

class UnaryExpr : public Expr {
 public:
  explicit UnaryExpr(TokenPtr operator_, ExprPtr right)
      : operator_(std::move(operator_)), right(std::move(right)) {}
  void accept(ExprVisitor& visitor) const override;

  TokenPtr operator_;
  ExprPtr right;
};

class VariableExpr : public Expr {
 public:
  explicit VariableExpr(TokenPtr name) : name(std::move(name)) {}
  void accept(ExprVisitor& visitor) const override;

  TokenPtr name;
};

class AssignExpr : public Expr {
 public:
  explicit AssignExpr(TokenPtr name, ExprPtr value)
      : name(std::move(name)), value(std::move(value)) {}
  void accept(ExprVisitor& visitor) const override;

  TokenPtr name;
  ExprPtr value;
};

class LogicalExpr : public Expr {
 public:
  explicit LogicalExpr(ExprPtr left, TokenPtr operator_, ExprPtr right)
      : left(std::move(left)),
        operator_(std::move(operator_)),
        right(std::move(right)) {}
  void accept(ExprVisitor& visitor) const override;

  ExprPtr left;
  TokenPtr operator_;
  ExprPtr right;
};

class CallExpr : public Expr {
 public:
  explicit CallExpr(ExprPtr callee, TokenPtr paren,
                    std::vector<ExprPtr> arguments)
      : callee(std::move(callee)),
        paren(std::move(paren)),
        arguments(std::move(arguments)) {}
  void accept(ExprVisitor& visitor) const override;

  // TODO: What exactly is callee? Prob a string as Exprs are only composed of
  // simple types, but confirm.
  ExprPtr callee;
  TokenPtr paren;
  std::vector<ExprPtr> arguments;
};

}  // namespace clox::Expr
