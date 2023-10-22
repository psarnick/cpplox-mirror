#pragma once
#include <memory>

#include "cpplox/Treewalk/Expr.h"
#include "cpplox/Treewalk/Token.h"

namespace clox::ExprFactory {
using namespace clox::Types;
using namespace clox::Expr;

ExprPtr make_binary_expr(ExprPtr left, const Token& operator_, ExprPtr right);
ExprPtr make_grouping_expr(ExprPtr expr);
// Needed because make_literal_expr("str") implicitly converts
// to bool in absence of const char* overload
ExprPtr make_literal_expr(const char* lit);
ExprPtr make_literal_expr(bool lit);
ExprPtr make_literal_expr(const Literal& lit);
ExprPtr make_unary_expr(const Token& operator_, ExprPtr right);
ExprPtr make_variable_expr(const Token& name);
ExprPtr make_assignment_expr(const Token& name, ExprPtr expr);
ExprPtr make_logical_expr(ExprPtr right, const Token& operator_, ExprPtr left);
ExprPtr make_call_expr(ExprPtr callee, const Token& paren,
                       std::vector<ExprPtr> args);
}  // namespace clox::ExprFactory
