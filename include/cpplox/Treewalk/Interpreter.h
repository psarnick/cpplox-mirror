#pragma once
#include <iostream>
#include <map>
#include <memory>
#include <variant>
#include <vector>

#include "cpplox/Treewalk/ErrorReporter.h"
#include "cpplox/Treewalk/Expr.h"
#include "cpplox/Treewalk/ExprVisitor.h"
#include "cpplox/Treewalk/Stmt.h"
#include "cpplox/Treewalk/StmtVisitor.h"
#include "cpplox/Treewalk/Token.h"

namespace clox {
using namespace ErrorsAndDebug;
using namespace clox::Expr;
using namespace clox::Stmt;
using namespace clox::Types;

// Types that Interpreter operates on.
class Callable;
using CallableSharedPtr = std::shared_ptr<Callable>;
using Value =
    std::variant<double, bool, std::string, CallableSharedPtr, std::monostate>;

class RuntimeException : public std::runtime_error {
 public:
  RuntimeException() : std::runtime_error(""), line_number(0){};
  RuntimeException(const std::string& msg, int ln)
      : std::runtime_error(msg.c_str()), line_number(ln) {}
  const int line_number{};
};

/*
 * Class thrown when "return" keyword is executed inside Lox Function. This
 * serves the purpose of unwinding call stack trace as control is returned to
 * Interpreter's method responsible for taking next action (interpret next
 * stmt/restore environment)
 */
class Return : RuntimeException {
 public:
  Return(Value val) : val(val) {}
  Value val;
};

class Interpreter;
/*
 * Based on the vector of values, Callable instructs Interpreter to run the
 * computation behind this callable. This primarily incorporates creating a new
 * environment and instructing interpretr to execute this callable's stmts in
 * this new env.
 *
 */
class Callable {
 public:
  virtual ~Callable(){};
  virtual Value call(Interpreter& intp, const std::vector<Value>& args) = 0;
  virtual int arity() = 0;
  virtual std::string to_string() = 0;
};

class Environment;
/*
 * Callable does not take ownership of FunctionStmt as this object is owned by
 * whoever owns the AST. To support closures, Function must have a valid handle
 * on its enclosing environment even after that environment is no longer used by
 * any other entity.
 */
class Function : public Callable {
 public:
  Function(const FunctionStmt& in_declaration, std::shared_ptr<Environment> env)
      : declaration(in_declaration),
        declarations_env(env),
        name("<fn " + declaration.name.get_lexeme() + ">") {}

  Value call(Interpreter& intp, const std::vector<Value>& args) override;
  int arity() override;
  std::string to_string() override { return name; }

 private:
  const FunctionStmt& declaration;
  std::shared_ptr<Environment> declarations_env;
  std::string name;
};

/*
 * Maintains state necessary to resolve names and scopes in runtime. To support
 * closures Environment must have a valid handle on its enclosing environment
 * even after that environment is no longer used by any other entity.
 * Environment owns the entries is stores. Right now value semantics is used to
 * pass Values around, hence owning the entries is trivial.
 *
 * TODO: flesh out what exactly happens in "for\\closure_in_body.lox" test.
 * TODO: Check when/if environments are copied as this impacts how/if map is
 * copied.
 */
class Environment {
 public:
  Environment(std::shared_ptr<Environment> enclosing_in)
      : enclosing(enclosing_in) {}

  explicit Environment() = default;
  ~Environment() = default;
  Environment(const Environment&) = default;
  Environment& operator=(const Environment&) = default;
  Environment(Environment&&) = default;
  // Environment operator=(Environment&&) = default;

  void define(std::string name, Value v);
  void assign(Token name, Value v);
  void assign_at_distance(Token name, Value v, int dist);
  Value get(Token name);
  Value get_at_distance(Token name, int distance);

 private:
  std::map<std::string, Value> values{};
  std::shared_ptr<Environment> enclosing;
};

/*
 * Based on AST represented by a vector of unique_ptrs, interpreter traverses
 *AST to run the code. Note: Despite accepting a vector of unique_ptrs
 *interpreter does not take ownership of the expressions and lifetime of those
 *is controlled by the caller. This is a flaw that could be fixed in the future
 *(see TODO).
 *
 * Interpreter bridges the gap between parser's static domain (ExprPtr and
 *StmtPtr) and runtime environment that is built on top a simpler data type -
 *Value.
 *
 * TODO: Clarify which environments Interpreter owns.
 * TODO: A more appropriate way of accepting statements would be via
 *vector<Stmt*> or even vector<Stmt&> if possible as this enforces the
 *non-ownership by interpreter. One way to do this is constructing ExprVisitor &
 *StmtVisitor class that traverses smart pointer AST and produces raw pointer
 *AST. Is there a better way? Current problem is that smart pointers leak
 *through the boundary into Interpreter.
 */
class Interpreter : public ExprVisitor, public StmtVisitor {
 public:
  Interpreter(ErrorReporter& e_reporter, std::ostream& out = std::cout)
      : val(Value{}), e_reporter(e_reporter), output(out) {
    global_env = std::make_shared<Environment>(nullptr);
    current_env = global_env;
    register_native_function();
  };

  void interpret(const std::vector<StmtPtr>& statements);
  void resolve(const clox::Expr::Expr& expr, int distance);

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

  // TODO: This is used by Env only. What about friend relationship here?
  void execute_block(const std::vector<StmtPtr>& statements,
                     std::shared_ptr<Environment> blocks_env);

  std::shared_ptr<Environment> current_env;
  std::shared_ptr<Environment> global_env;
  std::map<const clox::Expr::Expr*, int> locals{};

 private:
  // TODO: How to default initialize those well? Is constructor enough or
  // initialization below is needed too? TOOD: I think init below needed here
  // too as - happens if an implicitly defined constructor is called & val not
  // initialized?
  Value val{};
  ErrorReporter& e_reporter;
  std::ostream& output;

  void evaluate(const clox::Expr::Expr& expr);
  void register_native_function();
  Value lookup_variable(Token name, const clox::Expr::Expr& expr);
  bool is_truthy(const Value& val) const;
  bool is_equal(const Value& lhs, const Value& rhs);
  void ensure_number_operand(const Token& token, const Value& rhs);
  void ensure_number_operands(const Token& token, const Value& lhs,
                              const Value& rhs);
  template <typename T>
  bool same_types(const Value& lhs, const Value& rhs);
  template <typename T>
  T add(const Value& lhs, const Value& rhs);
};
}  // namespace clox
