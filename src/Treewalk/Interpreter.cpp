#include "cpplox/Treewalk/Interpreter.h"

#include <chrono>
#include <iostream>
#include <sstream>

/*
TOOD: Organise what exceptions this code throws? There's a mix of custom & std::
ones.
*/
namespace clox {

using clox::Types::literal_to_string;
using clox::Types::TokenType;

std::string value_as_string(const Value& val) {
  // TODO: Code duplicated from Literal.cpp - how to refactor into one?
  using namespace std;
  if (holds_alternative<bool>(val)) {
    return get<bool>(val) ? "true" : "false";
  }

  if (holds_alternative<double>(val)) {
    stringstream stream;
    stream << get<double>(val);
    return stream.str();
  }

  if (holds_alternative<string>(val)) {
    return get<string>(val);
  }

  if (holds_alternative<monostate>(val)) {
    return "nil";
  }

  if (holds_alternative<CallableSharedPtr>(val)) {
    return get<CallableSharedPtr>(val)->to_string();
  }

  throw runtime_error("non-exhaustive visitor");
}

void Environment::define(std::string name, Value v) { values[name] = v; }

void Environment::assign(Token name, Value v) {
  if (values.find(name.get_lexeme()) != values.end()) {
    values[name.get_lexeme()] = v;
    return;
  }

  if (enclosing != nullptr) {
    enclosing->assign(name, v);
    return;
  }

  throw RuntimeException("Undefined variable '" + name.get_lexeme() + "'.",
                         name.get_line());
}

Value Environment::get_at_distance(Token name, int dist) {
  Environment* e = this;
  while (dist > 0) {
    e = e->enclosing.get();
    dist--;
  }
  return e->get(name);
}

void Environment::assign_at_distance(Token name, Value v, int dist) {
  Environment* e = this;
  while (dist > 0) {
    e = e->enclosing.get();
    dist--;
  }
  e->assign(name, v);
}

Value Environment::get(Token name) {
  if (values.find(name.get_lexeme()) != values.end()) {
    Value v = values.at(name.get_lexeme());
    return v;
  }

  if (enclosing != nullptr) {
    return enclosing->get(name);
  }

  throw RuntimeException("Undefined variable '" + name.get_lexeme() + "'.",
                         name.get_line());
}

void Interpreter::evaluate(const clox::Expr::Expr& expr) { expr.accept(*this); }

void Interpreter::interpret(const std::vector<StmtPtr>& statements) {
  try {
    for (auto const& s : statements) {
      s->accept(*this);
    }
  } catch (RuntimeException e) {
    e_reporter.set_error("[line " + std::to_string(e.line_number) +
                         "] while interpreting: " + e.what());
    return;
  }
}

void Interpreter::resolve(const clox::Expr::Expr& expr, int distance) {
  locals.insert({&expr, distance});
}

void Interpreter::visitBinary(const BinaryExpr& binary_expr) {
  evaluate(*binary_expr.left);
  Value lhs{val};

  evaluate(*binary_expr.right);
  Value rhs{val};

  const Token& token{*binary_expr.operator_};
  switch (binary_expr.operator_->get_type()) {
    case TokenType::GREATER:
      ensure_number_operands(token, lhs, rhs);
      val = std::get<double>(lhs) > std::get<double>(rhs);
      break;
    case TokenType::GREATER_EQUAL:
      ensure_number_operands(token, lhs, rhs);
      val = std::get<double>(lhs) >= std::get<double>(rhs);
      break;
    case TokenType::LESS:
      ensure_number_operands(token, lhs, rhs);
      val = std::get<double>(lhs) < std::get<double>(rhs);
      break;
    case TokenType::LESS_EQUAL:
      ensure_number_operands(token, lhs, rhs);
      val = std::get<double>(lhs) <= std::get<double>(rhs);
      break;
    case TokenType::MINUS:
      ensure_number_operands(token, lhs, rhs);
      val = std::get<double>(lhs) - std::get<double>(rhs);
      break;
    case TokenType::SLASH:
      ensure_number_operands(token, lhs, rhs);
      val = std::get<double>(lhs) / std::get<double>(rhs);
      break;
    case TokenType::STAR:
      ensure_number_operands(token, lhs, rhs);
      val = std::get<double>(lhs) * std::get<double>(rhs);
      break;
    case TokenType::BANG_EQUAL:
      val = std::move(!is_equal(lhs, rhs));
      break;
    case TokenType::EQUAL_EQUAL:
      val = is_equal(lhs, rhs);
      break;
    case TokenType::PLUS:
      if (same_types<double>(lhs, rhs)) {
        val = add<double>(lhs, rhs);
        return;
      }

      if (same_types<std::string>(lhs, rhs)) {
        val = add<std::string>(lhs, rhs);
        return;
      }

      throw RuntimeException("Operands must be two numbers or strings.",
                             token.get_line());
      break;
    case TokenType::AND:
    case TokenType::BANG:
    case TokenType::CLASS:
    case TokenType::COMMA:
    case TokenType::DOT:
    case TokenType::ELSE:
    case TokenType::EQUAL:
    case TokenType::FALSE:
    case TokenType::FOR:
    case TokenType::FUN:
    case TokenType::IDENTIFIER:
    case TokenType::IF:
    case TokenType::LEFT_BRACE:
    case TokenType::LEFT_PAREN:
    case TokenType::LOX_EOF:
    case TokenType::NIL:
    case TokenType::NUMBER:
    case TokenType::OR:
    case TokenType::PRINT:
    case TokenType::RETURN:
    case TokenType::RIGHT_BRACE:
    case TokenType::RIGHT_PAREN:
    case TokenType::SEMICOLON:
    case TokenType::STRING:
    case TokenType::SUPER:
    case TokenType::THIS:
    case TokenType::TRUE:
    case TokenType::VAR:
    case TokenType::WHILE:
      throw std::logic_error("Unexpected binary operator: " +
                             binary_expr.operator_->to_string());
  }
}

void Interpreter::visitLiteral(const LiteralExpr& literal_expr) {
  // TODO: Possible to do with templates/visitor?
  if (std::holds_alternative<double>(*literal_expr.literal)) {
    val = std::get<double>(*literal_expr.literal);
  } else if (std::holds_alternative<bool>(*literal_expr.literal)) {
    val = std::get<bool>(*literal_expr.literal);
  } else if (std::holds_alternative<std::string>(*literal_expr.literal)) {
    val = std::get<std::string>(*literal_expr.literal);
  } else if (std::holds_alternative<std::monostate>(*literal_expr.literal)) {
    val = std::get<std::monostate>(*literal_expr.literal);
  } else {
    throw std::runtime_error("Non-exhaustive visitLiteral copy");
  }
}

void Interpreter::visitGrouping(const GroupingExpr& grouping) {
  evaluate(*grouping.expression);
}

void Interpreter::visitUnary(const UnaryExpr& unary) {
  evaluate(*unary.right);
  switch (unary.operator_->get_type()) {
    case TokenType::MINUS:
      ensure_number_operand(*unary.operator_, val);
      val = -std::get<double>(val);
      break;
    case TokenType::BANG:
      val = !is_truthy(val);
      break;
    case TokenType::AND:
    case TokenType::BANG_EQUAL:
    case TokenType::CLASS:
    case TokenType::COMMA:
    case TokenType::DOT:
    case TokenType::ELSE:
    case TokenType::EQUAL:
    case TokenType::EQUAL_EQUAL:
    case TokenType::FALSE:
    case TokenType::FOR:
    case TokenType::FUN:
    case TokenType::GREATER:
    case TokenType::GREATER_EQUAL:
    case TokenType::IDENTIFIER:
    case TokenType::IF:
    case TokenType::LEFT_BRACE:
    case TokenType::LEFT_PAREN:
    case TokenType::LESS:
    case TokenType::LESS_EQUAL:
    case TokenType::LOX_EOF:
    case TokenType::NIL:
    case TokenType::NUMBER:
    case TokenType::OR:
    case TokenType::PLUS:
    case TokenType::PRINT:
    case TokenType::RETURN:
    case TokenType::RIGHT_BRACE:
    case TokenType::RIGHT_PAREN:
    case TokenType::SEMICOLON:
    case TokenType::SLASH:
    case TokenType::STAR:
    case TokenType::STRING:
    case TokenType::SUPER:
    case TokenType::THIS:
    case TokenType::TRUE:
    case TokenType::VAR:
    case TokenType::WHILE:
      throw std::logic_error("Unexpected binary operator: " +
                             unary.operator_->to_string());
  }
}

void Interpreter::visitVariable(const VariableExpr& var) {
  // val = current_env->get(*var.name);
  val = lookup_variable(*var.name, var);
}

Value Interpreter::lookup_variable(Token name, const clox::Expr::Expr& expr) {
  if (locals.find(&expr) == locals.end()) {
    return global_env->get(name);
  } else {
    return current_env->get_at_distance(name, locals.find(&expr)->second);
  }
}

void Interpreter::visitAssign(const AssignExpr& assign) {
  evaluate(*assign.value);
  // current_env->assign(*assign.name, val);
  if (locals.find(&assign) == locals.end()) {
    global_env->assign(*assign.name, val);
  } else {
    int dist = locals.find(&assign)->second;
    current_env->assign_at_distance(*assign.name, val, dist);
  }
}

void Interpreter::visitLogical(const LogicalExpr& logical) {
  evaluate(*logical.left);
  TokenType t{logical.operator_->get_type()};
  // True or <sth> shortcircuits to is_truthy
  // False and <sth> shortcircuits to it_truthy
  if ((is_truthy(val) && t == TokenType::OR) ||
      (!is_truthy(val) && t == TokenType::AND)) {
    return;
  }

  // At this it's one of: a) False or val b) True and val. In both cases rhs
  // dictates.
  evaluate(*logical.right);
}

void Interpreter::visitExpression(const ExpressionStmt& expr) {
  evaluate(*expr.expr);
}

void Interpreter::visitPrint(const PrintStmt& print) {
  evaluate(*print.expr);
  output << value_as_string(val) << std::endl;
}

void Interpreter::visitVar(const VarStmt& var) {
  Value init{std::monostate()};
  if (var.initializer) {
    evaluate(*var.initializer);
    init = val;
  }
  current_env->define(var.name.get_lexeme(), init);
}

void Interpreter::visitBlock(const BlockStmt& block) {
  execute_block(block.statements, std::make_shared<Environment>(current_env));
}

void Interpreter::visitIf(const IfStmt& if_) {
  evaluate(*if_.condition);
  if (is_truthy(val)) {
    // TODO: Have plumbing to interpret a single statement
    if_.then_branch->accept(*this);
  } else if (if_.else_branch) {
    if_.else_branch->accept(*this);
  }
}

void Interpreter::visitWhile(const WhileStmt& while_) {
  evaluate(*while_.condition);
  while (is_truthy(val)) {
    // TODO: Have plumbing to interpret a single statement
    while_.body->accept(*this);
    evaluate(*while_.condition);
  }
}

// TODO: start using "override"
void Interpreter::visitCall(const CallExpr& call) {
  evaluate(*call.callee);
  Value callee{val};

  std::vector<Value> arguments{};
  for (auto& a : call.arguments) {
    evaluate(*a);
    arguments.push_back(val);
  }

  if (!std::holds_alternative<CallableSharedPtr>(callee)) {
    throw RuntimeException("Only functions and classes can be called.",
                           call.paren->get_line());
  }

  CallableSharedPtr callable_ptr = std::get<CallableSharedPtr>(callee);
  if (callable_ptr->arity() != arguments.size()) {
    throw RuntimeException(
        "Expected: " + std::to_string(callable_ptr->arity()) +
            " arguments, but got: " + std::to_string(arguments.size()),
        call.paren->get_line());
  }
  val = callable_ptr->call(*this, arguments);
}

void Interpreter::visitFunction(const FunctionStmt& declaration_stmt) {
  current_env->define(
      declaration_stmt.name.get_lexeme(),
      std::make_shared<Function>(declaration_stmt, current_env));
}

void Interpreter::visitReturn(const ReturnStmt& return_) {
  if (return_.value) {
    evaluate(*return_.value);
  } else {
    val = std::monostate();
  }
  throw Return(val);
}

void Interpreter::execute_block(const std::vector<StmtPtr>& statements,
                                std::shared_ptr<Environment> blocks_env) {
  // TODO: Is this exception safe?
  std::shared_ptr<Environment> to_restore{current_env};
  current_env = blocks_env;

  try {
    interpret(statements);
  } catch (Return ret) {
    current_env = to_restore;
    throw ret;
  };
  current_env = to_restore;
}

bool Interpreter::is_truthy(const Value& val) const {
  if (std::holds_alternative<std::monostate>(val)) {
    return false;
  }

  if (std::holds_alternative<bool>(val)) {
    return std::get<bool>(val);
  }
  return true;
}

bool Interpreter::is_equal(const Value& lhs, const Value& rhs) {
  // 2 std::monostate variants compare equal.
  return lhs == rhs;
}

void Interpreter::ensure_number_operand(const Token& token, const Value& rhs) {
  if (std::holds_alternative<double>(rhs)) {
    return;
  }
  throw RuntimeException("Operand must be a number.", token.get_line());
}

void Interpreter::ensure_number_operands(const Token& token, const Value& lhs,
                                         const Value& rhs) {
  if (std::holds_alternative<double>(lhs) &&
      std::holds_alternative<double>(rhs)) {
    return;
  }
  throw RuntimeException("Operands must be numbers.", token.get_line());
}

template <typename T>
bool Interpreter::same_types(const Value& lhs, const Value& rhs) {
  return std::holds_alternative<T>(lhs) && std::holds_alternative<T>(rhs);
}

template <typename T>
T Interpreter::add(const Value& lhs, const Value& rhs) {
  return std::get<T>(lhs) + std::get<T>(rhs);
}

void Interpreter::register_native_function() {
  class BuiltInClock : public Callable {
   private:
    Value call(Interpreter& intp, const std::vector<Value>& args) {
      return static_cast<double>(
          std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::high_resolution_clock::now().time_since_epoch())
              .count());
    }
    int arity() { return 0; }
    std::string to_string() { return "<fn clock>"; }
  };
  global_env->define("clock", std::make_shared<BuiltInClock>());
}

Value Function::call(Interpreter& intp, const std::vector<Value>& args) {
  std::shared_ptr<Environment> functions_env{
      std::make_shared<Environment>(this->declarations_env)};
  for (size_t i = 0; i < declaration.params.size(); ++i) {
    functions_env->define(declaration.params[i].get_lexeme(), args[i]);
  }
  try {
    intp.execute_block(declaration.body, functions_env);
  } catch (Return ret) {
    return ret.val;
  };
  return std::monostate();
}

int Function::arity() { return declaration.params.size(); }
};  // namespace clox