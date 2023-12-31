#include "cpplox/Bytecode/Compiler.h"

#include <stdexcept>

#include "cpplox/Bytecode/Debug.h"
#include "cpplox/Bytecode/common.h"
#include "cpplox/Treewalk/Scanner.h"
// TOOD: Move scanner to common / scanner package

namespace cpplox {

std::optional<function_ptr> Compiler::compile() {
  if (!healthy) {
    throw std::logic_error(
        "Compiler not designed to be called multiple times, create a new "
        "instance.");
  }
  register_gc_callbacks();
  healthy = false;
  current = 0;
  had_error = false;
  panic_mode = false;
  while (current < tokens.size() && !match(TokenType::LOX_EOF)) {
    declaration();
  }
  if (tokens[previous].get_type() != TokenType::LOX_EOF) {
    error_at(previous, "Expected end of file.");
  }
  end_compiler();
  return had_error ? std::nullopt : std::optional<function_ptr>(function);
}

void Compiler::advance() {
  previous = current;
  current++;
}

void Compiler::declaration() {
  // Separate rule for statements that define names which are only allowed
  // on top level or inside of a block.
  // See: https://craftinginterpreters.com/global-variables.html#statements
  if (match(TokenType::VAR)) {
    var_declaration();
  } else if (match(TokenType::FUN)) {
    dispatch_function_declaration();
  } else {
    statement();
  }
  if (panic_mode) synchronize();
}

void Compiler::var_declaration() {
  uint8_t maybe_const_table_index_of_global_variable_name =
      parse_variable("Expected variable name after 'var'.");
  if (match(TokenType::EQUAL)) {
    expression();  // initialiser expression
  } else {
    emit_opcode(OpCode::OP_NIL);  // default initialisation
  }
  consume(TokenType::SEMICOLON, "Expected ; after variable declaration.");
  define_variable(maybe_const_table_index_of_global_variable_name);
}

void Compiler::dispatch_function_declaration() {
  uint8_t maybe_const_table_index_of_global_variable_name =
      parse_variable("Expected function name after 'fun'.");
  if (scope_depth > 0) {
    locals.back().depth = scope_depth;
    locals.back().ready = true;
  }
  // Finalizign local var referring to function name as recursive
  // functions refer to their name after declaration but before definition.
  // This will not lead to using incomplete state as function declaration will
  // be fully parsed before function can actually run.
  // TODO: ^ is this even needed? Think if functions are always global in their
  // own Compiler.

  Compiler function_compiler{tokens, disassembler, e_reporter, heap, pool, current, this};
  function_compiler.register_gc_callbacks();
  // TODO: Move to ctor and dctor.
  function_compiler.function_declaration();
  function_compiler.end_compiler();
  // Instead of managing recursive state in a single Compiler object, create a
  // a new compiler to process each function & then steal the bytecode it generated.
  
  emit_closure(function_compiler.function);
  for (auto& upvalue : function_compiler.upvalues) {
    emit_operand(upvalue.is_local ? 1 : 0);
    emit_operand(upvalue.index);
  }

  define_variable(maybe_const_table_index_of_global_variable_name);
  current = function_compiler.current;
  previous = function_compiler.previous;
  had_error = function_compiler.had_error;
  panic_mode = function_compiler.panic_mode;
}

void Compiler::function_declaration() {
  begin_scope();
  // Wrapping function declaration in a scope to avoid top-level function
  // parameters from polluting global namespace and to shadow outer variables.
  consume(TokenType::LEFT_PAREN, "Expected '(' after function name.");
  if (!check(TokenType::RIGHT_PAREN)) {
    do {
      if (function->arity >= std::numeric_limits<uint8_t>::max()) {
        error_at_current("Functions accept at most 255 parameters.");
        return;
      }
      function->arity++;
      uint8_t zero_as_function_args_are_local =
          parse_variable("Expected variable name.");
      assert(zero_as_function_args_are_local == 0);
      define_variable(zero_as_function_args_are_local);
    } while (match(TokenType::COMMA));
  }
  consume(TokenType::RIGHT_PAREN,
          "Expected ')' after function parameter list.");
  consume(TokenType::LEFT_BRACE, "Expected '{' as function body definition.");
  block();
  healthy = false;
  // Invoking Compiler::end_scope would emit OP_POP for all local variables
  // in current scope, including the return value from this function.
  // Instead, the responsibility to clean up stack is offloaded to VM, which can
  // hold on to the return value during the process. Skipping the call to
  // Compiler::end_scope leaves the compiler in a bad state, so setting the
  // flag.
}

void Compiler::statement() {
  // Separate rule for statements allowed inside control flow body.
  // See: https://craftinginterpreters.com/global-variables.html#statements
  if (match(TokenType::PRINT)) {
    print_statement();
  } else if (match(TokenType::LEFT_BRACE)) {
    begin_scope();
    block();
    end_scope();
  } else if (match(TokenType::IF)) {
    if_statement();
  } else if (match(TokenType::WHILE)) {
    while_statement();
  } else if (match(TokenType::FOR)) {
    for_statement();
  } else if (match(TokenType::RETURN)) {
    return_statement();
  } else {
    expression_statement();
  }
}

void Compiler::expression_statement() {
  // Expressions have a stack effect of +1 (they produce a value on the stack),
  // but statements leave stack unchanged. As such, expression statement
  // evaluates to the underlying expression and discards the result (OP_POP at
  // the end).
  expression();
  consume(TokenType::SEMICOLON, "Expression statements must end with ';'");
  emit_opcode(OpCode::OP_POP);
}

void Compiler::print_statement() {
  expression();
  consume(TokenType::SEMICOLON, "Expecting ; after print statement.");
  emit_opcode(OpCode::OP_PRINT);
}

void Compiler::block() {
  while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::LOX_EOF)) {
    declaration();
  }
  consume(TokenType::RIGHT_BRACE, "Expecting } after block.");
}

void Compiler::expression() {
  parse_precedence(Precedence::ASSIGNMENT);
  // Start parsing with second-to-lowest precedence level and stay/go up from
  // here.
}

void Compiler::number(const bool precedence_context_allows_assignment) {
  if (!std::holds_alternative<double>(tokens[previous].get_literal())) {
    throw std::logic_error("Only floating point numbers supported for now.");
  }
  double val{std::get<double>(tokens[previous].get_literal())};
  emit_constant(val);
}

void Compiler::string(const bool precedence_context_allows_assignment) {
  const std::string& lexeme = tokens[previous].get_lexeme();
  emit_constant({pool->insert_or_get(lexeme.substr(1, lexeme.length() - 2))});  // skip "
}

void Compiler::grouping(const bool precedence_context_allows_assignment) {
  expression();
  consume(TokenType::RIGHT_PAREN, "Expecting right paren ) after expression.");
}

void Compiler::unary(const bool precedence_context_allows_assignment) {
  const Token& token{tokens[previous]};
  parse_precedence(Precedence::UNARY);
  switch (token.get_type()) {
    case TokenType::BANG:
      emit_opcode(OpCode::OP_NOT);
      break;
    case TokenType::MINUS:
      emit_opcode(OpCode::OP_NEGATE);
      break;
    default:
      throw std::runtime_error("Unknown op in unary: " + token.to_string());
  }
}

void Compiler::binary(const bool precedence_context_allows_assignment) {
  const Token& token = tokens[previous];
  TokenType op{token.get_type()};
  const ParseRule& current_rule = rules.at(static_cast<size_t>(op));
  Precedence one_higher =
      static_cast<Precedence>(static_cast<size_t>(current_rule.precedence) + 1);
  parse_precedence(one_higher);
  // As binary operators are left-associative, parsing this op's right-hand
  // operand has to be constrained to only include more binding operators.
  // Examples: when compiling RHS of * in 2 * 3 + 4, just the token "3" should
  // be included, not "3", "+" and "4"; when compiling RHS of * in 2 *
  // instance.call() tokens for "instance", "." and "call()" should be consumed.

  switch (op) {
    case TokenType::BANG_EQUAL:
      emit_opcodes(OpCode::OP_EQUAL, OpCode::OP_NOT);
      break;
    case TokenType::EQUAL_EQUAL:
      emit_opcode(OpCode::OP_EQUAL);
      break;
    case TokenType::GREATER:
      emit_opcode(OpCode::OP_GREATER);
      break;
    case TokenType::GREATER_EQUAL:
      emit_opcodes(OpCode::OP_LESS, OpCode::OP_NOT);
      break;
    case TokenType::LESS:
      emit_opcode(OpCode::OP_LESS);
      break;
    case TokenType::LESS_EQUAL:
      emit_opcodes(OpCode::OP_GREATER, OpCode::OP_NOT);
      break;
    case TokenType::STAR:
      emit_opcode(OpCode::OP_MULTIPLY);
      break;
    case TokenType::SLASH:
      emit_opcode(OpCode::OP_DIVIDE);
      break;
    case TokenType::PLUS:
      emit_opcode(OpCode::OP_ADD);
      break;
    case TokenType::MINUS:
      emit_opcode(OpCode::OP_SUBTRACT);
      break;
    default:
      throw std::logic_error("unknown op: " + token.to_string());
  }
}

void Compiler::and_(const bool precedence_context_allows_assignment) {
  // When and_ is called LHS is evaluated and on the stack, if value is:
  // falsey:  VM skips RHS and leaves LHS value as the result for the entire
  // expression.
  // !falsey: VM pops LHS value and evaluates RHS, which becomes the
  // result.
  uint16_t jump_over_rhs_instr_idx = emit_jump(OpCode::OP_JUMP_IF_FALSE);
  emit_opcode(OpCode::OP_POP);
  parse_precedence(Precedence::AND);
  patch_jump(jump_over_rhs_instr_idx);
}

void Compiler::or_(const bool precedence_context_allows_assignment) {
  // When or_ is called LHS is evaluated and on the stack, if value is:
  // falsey:  VM pops LHS value and evaluates RHS, which becomes the result.
  // !falsey: VM skips RHS and leaves LHS value as the result for the entire
  // expression. TODO optimisation idea: add OP_JUMP_IF_TRUE as currently
  // "or" is slower than "and".
  uint16_t jump_to_rhs_instr_idx = emit_jump(OpCode::OP_JUMP_IF_FALSE);
  // Jumps over the jump_over_rhs jump. Parse that!
  uint16_t jump_over_rhs_instr_idx = emit_jump(OpCode::OP_JUMP);
  patch_jump(jump_to_rhs_instr_idx);
  emit_opcode(OpCode::OP_POP);
  parse_precedence(Precedence::OR);
  patch_jump(jump_over_rhs_instr_idx);
}

void Compiler::literal(const bool precedence_context_allows_assignment) {
  const Token& token = tokens[previous];
  TokenType ttype{token.get_type()};
  switch (ttype) {
    case TokenType::FALSE:
      emit_opcode(OpCode::OP_FALSE);
      break;
    case TokenType::TRUE:
      emit_opcode(OpCode::OP_TRUE);
      break;
    case TokenType::NIL:
      emit_opcode(OpCode::OP_NIL);
      break;
    default:
      throw std::runtime_error("unknown op: " + token.to_string());
  }
}

void Compiler::call(const bool precedence_context_allows_assignment) {
  int arg_count{0};
  if (!check(TokenType::RIGHT_PAREN)) {
    do {
      if (arg_count >= std::numeric_limits<uint8_t>::max()) {
        error_at_current("Cannot have more than 255 arguments.");
        return;
      }
      arg_count++;
      expression();
      // Each argument is an expresssion and leaves one value on the stack.
    } while (match(TokenType::COMMA));
  }
  consume(TokenType::RIGHT_PAREN, "Expected ')' at the end of function call.");
  emit_opcode(OpCode::OP_CALL);
  emit_operand(arg_count);
}

void Compiler::return_statement() {
  if (function->name == pool->insert_or_get("script")) {
    error_at(previous, "Cannot return from top-level code.");
  }
  if (match(TokenType::SEMICOLON)) {
    emit_return();  // implicit return nil;
  } else {
    expression();
    consume(TokenType::SEMICOLON, "Expected ';' after return value.");
    emit_opcode(OpCode::OP_RETURN);
  }
}

void Compiler::variable(const bool precedence_context_allows_assignment) {
  named_variable(tokens[previous].get_lexeme(),
                 precedence_context_allows_assignment);
}

void Compiler::parse_precedence(const Precedence& precedence) {
  // Start at current token and parse expressions with equal-or-higher
  // precedence as defined in Precedence enum.
  size_t token_type_as_number = static_cast<size_t>(tokens[current].get_type());
  ParseFn prefix_fn = rules[token_type_as_number].prefix;
  if (prefix_fn == nullptr) {
    error_at_current("Expected expression.");
    return;
  }
  advance();
  bool precedence_context_allows_assignment =
      precedence <= Precedence::ASSIGNMENT;
  // Downstream parsing functions need to know if this expression's precedence
  // allows assignment, example: in "a * b = c + d;" named_variable that parses
  // "b" would peek() and see subsequent "=" incorrectly treating "a * b" as a
  // valid assignment target. This flag informs parsing functions is surrounding
  // precedence is low enough to consume "=" during parsing.
  (this->*prefix_fn)(precedence_context_allows_assignment);
  // When reading valid code left to right, the first token is always belongs
  // to some prefix expression. Best to step in debugger to see how it  works.

  token_type_as_number = static_cast<size_t>(tokens[current].get_type());
  while (precedence <= rules[token_type_as_number].precedence) {
    advance();
    (this->*rules[token_type_as_number].infix)(
        precedence_context_allows_assignment);
    token_type_as_number = static_cast<size_t>(tokens[current].get_type());
  }
  if (precedence_context_allows_assignment && match(TokenType::EQUAL)) {
    // Despite the flag being true, children parsing functions finished without
    // consuming "=". At this point (outside of an expression) nothing will
    // consume "=" as it does not define prefix or infix parsing functions.
    error_at(previous, "Invalid assignment target.");
  }
}

uint8_t Compiler::parse_variable(const std::string& err_msg) {
  consume(TokenType::IDENTIFIER, err_msg);
  declare_variable();
  if (scope_depth == 0) {
    return add_constant(pool->insert_or_get(tokens[previous].get_lexeme()));
    // Globals are resolved dynamically - write var name to constants table &
    // return its index
  } else {
    return 0;
    // Locals are looked up by stack index so there’s no need to store
    // the variable’s name into the constant table. Return a dummy table index
    // instead, which is ignored in ::define_variable.
  }
}

void Compiler::declare_variable() {
  if (scope_depth == 0) return;
  // Globals are lazy-bound, so compiler doesn't need to keep track which
  // declarations is has seen. This means global variables can be redeclared
  // (useful for REPL).

  const std::string& var_name = tokens[previous].get_lexeme();
  for (auto it = locals.crbegin(); it != locals.crend(); it++) {
    if (it->depth < scope_depth) {
      break;  // break as already in parent scope
    }
    if (it->name == var_name) {
      // TODO optimisation: could intern strings & compare pointers.
      error_at(previous, "Variable with this name already in scope.");
    }
    // This loop ensures local variables cannot be redefined: { var a = 1; var a
    // = 2; } Shadowing is ok though: { var a = 1; { var a = 2; } }
  }

  if (locals.size() > std::numeric_limits<uint8_t>::max()) {
    error_at(previous, "Too many local variables.");
    // VM limitation: the instructions working with local variables refer to
    // them by a 1 byte slot index. Exceeding 256 variables (max representable
    // id is 255) would break addressing.
    return;
  }
  locals.push_back({.name = var_name, .depth = -1, .ready = false, .is_captured = false});
  // Local is declared but must not be resolved as its initializer is yet to be
  // parsed.
}

void Compiler::define_variable(
    const uint8_t const_table_index_of_global_variable_name) {
  if (scope_depth > 0) {
    locals.back().depth = scope_depth;
    locals.back().ready = true;
    return;
    // Local variable's initializer is compiled and local is ready for use.
    // Bytecode isn't needed to create a local var at runtime. The VM has just
    // executed the initializer and the local variable's value is on top of the
    // stack. Temporary slot becomes the local variable.
  } else {
    // Global variables are looked up by name at runtime, but entire variable
    // name is too large to store as bytecode operand - store it in constants
    // table and make bytecode refer to the global var by index instead.
    // TODO: clox's reference runtime implementation looks up globals by string
    // name; but it also already identifies them by array index (here), so it
    // should be possible to use vector instead of map (unless problems in
    // later chapters).
    emit_opcode(OpCode::OP_DEFINE_GLOBAL);
    emit_operand(const_table_index_of_global_variable_name);
  }
}

void Compiler::begin_scope() { scope_depth++; }

void Compiler::end_scope() {
  scope_depth--;
  int cnt = 0;
  for (auto it = locals.crbegin(); it != locals.crend(); it++) {
    if (it->depth <= scope_depth) {
      break;
    }
    if (it->is_captured) {
      emit_opcode(OpCode::OP_CLOSE_UPVALUE);
    } else {
      emit_opcode(OpCode::OP_POP);
      // TODO optimisation: OP_POPN that takes an operant & pops N slots at once.
    }
    cnt++;
  }
  locals.erase(locals.end() - cnt, locals.end());
  // TODO: Possible optimisation - max size of locals is known, so I could
  // preallocate & reuse objects.
}

void Compiler::if_statement() {
  consume(TokenType::LEFT_PAREN, "Expected '(' after if");
  expression();
  consume(TokenType::RIGHT_PAREN, "Expected ')' after condition");
  uint16_t jump_over_if_branch_instr_idx = emit_jump(OpCode::OP_JUMP_IF_FALSE);
  // Conditionally jump over the "if" branch.
  emit_opcode(OpCode::OP_POP);
  // Evaluating the "if" condition leaves value on stack that has to be cleaned
  // up. This POP will be reached only if the condition is true (= no jump).
  statement();
  uint16_t jump_over_else_branch_instr_idx = emit_jump(OpCode::OP_JUMP);
  // Unconditionally jump over the the "else" branch.
  patch_jump(jump_over_if_branch_instr_idx);
  // "if" jump should land after last instruction in "if" branch.
  emit_opcode(OpCode::OP_POP);
  // Just like the POP above, but reached only if the condition is false (=
  // jump).
  if (match(TokenType::ELSE)) {
    statement();
  }
  patch_jump(jump_over_else_branch_instr_idx);
  // "else" jump should land after the optional else clause.
}

void Compiler::while_statement() {
  size_t loop_start_instr_idx = function->chunk->code.size();
  consume(TokenType::LEFT_PAREN, "Expected '(' after while");
  expression();
  consume(TokenType::RIGHT_PAREN, "Expected ')' after condition");
  uint16_t jump_over_while_body_instr_idx = emit_jump(OpCode::OP_JUMP_IF_FALSE);
  emit_opcode(OpCode::OP_POP);
  statement();
  emit_loop(loop_start_instr_idx);
  patch_jump(jump_over_while_body_instr_idx);
  emit_opcode(OpCode::OP_POP);
}

void Compiler::for_statement() {
  begin_scope();
  consume(TokenType::LEFT_PAREN, "Expected '(' after for");
  if (match(TokenType::VAR)) {
    var_declaration();
  } else if (match(TokenType::SEMICOLON)) {
    // Noop as initializer was not specified. Match moves to the next token.
  } else {
    expression_statement();
  }
  // 1st ';' consumed above, no side-effect on stack.
  size_t loop_start_instr_idx = function->chunk->code.size();
  // Every loop iteration starts with evaluating the condition.

  uint16_t maybe_jump_over_loop_body_instr_idx{0};
  bool had_condition{false};
  if (!check(TokenType::SEMICOLON)) {
    expression();
    had_condition = true;
    maybe_jump_over_loop_body_instr_idx = emit_jump(OpCode::OP_JUMP_IF_FALSE);
    emit_opcode(
        OpCode::OP_POP);  // pop condition expression's value on "true" branch.
  }
  consume(TokenType::SEMICOLON, "for loop condition must be followed by ';'");

  if (!check(TokenType::RIGHT_PAREN)) {
    // Increment clause should be evaluated at the end of each loop iteration.
    // https://craftinginterpreters.com/jumping-back-and-forth.html#increment-clause
    // has a helpful diagram.
    uint16_t jump_over_incr_expr_instr_idx = emit_jump(OpCode::OP_JUMP);
    uint16_t maybe_increment_expr_instr_idx = function->chunk->code.size();
    expression();
    emit_opcode(OpCode::OP_POP);
    emit_loop(loop_start_instr_idx);
    loop_start_instr_idx = maybe_increment_expr_instr_idx;
    // After evaluating increment clause, resume from the for loop condition.
    patch_jump(jump_over_incr_expr_instr_idx);
  }
  consume(TokenType::RIGHT_PAREN, "for loop clauses must be followed by ')");
  statement();
  emit_loop(loop_start_instr_idx);

  if (had_condition) {
    patch_jump(maybe_jump_over_loop_body_instr_idx);
    emit_opcode(
        OpCode::OP_POP);  // pop condition expression's value on "false" branch.
  }
  end_scope();
}

void Compiler::named_variable(const std::string var_name,
                              const bool precedence_context_allows_assignment) {
  // TODO: Replace var_name with string_view.



  auto [idx_if_found, found] = resolve_local(var_name);
  OpCode get_op;  // Op to emit if named variable is read from.
  OpCode set_op;  // Op to emit if named variable is written to.
  uint8_t idx{0};
  if (found) {
    idx = idx_if_found;
    get_op = OpCode::OP_GET_LOCAL;
    set_op = OpCode::OP_SET_LOCAL;
  } else {
    // TODO: Make this flatter.
    std::tie(idx_if_found, found) = resolve_upvalue(var_name);
    if (found) {
      idx = idx_if_found;
      get_op = OpCode::OP_GET_UPVALUE;
      set_op = OpCode::OP_SET_UPVALUE;
    } else {
      idx = add_constant(pool->insert_or_get(var_name));
      // TODO: This always adds gloabl variables to constants pool, even when
      // they are read.
      // TODO: Challenge at
      // https://craftinginterpreters.com/global-variables.html#reading-variables
      // Store global variable name in function chunk's constants pool.
      get_op = OpCode::OP_GET_GLOBAL;
      set_op = OpCode::OP_SET_GLOBAL;
    }
  }

  if (precedence_context_allows_assignment && match(TokenType::EQUAL)) {
    // This identifier expression is target for a valid variable assignment,
    // example: a.call().y = x;, Note: a.call().y == x; would produce
    // TokenType::EQUAL_EQUAL.
    expression();
    emit_opcode(set_op);
  } else {
    emit_opcode(get_op);
  }
  emit_operand(idx);
}

std::pair<uint8_t, bool> Compiler::resolve_local(const std::string& name) {
  // At runtime, locals are loaded and stored using the stack slot index, so
  // that’s what the compiler needs to calculate when it resolves the variable.
  // Luckily, locals vector has the same layout at VM's stack will have at
  // runtime. TODO optimisation: avoid linear scan, possibly by holding a map
  // from string -> vector of indices.
  size_t idx = locals.size() - 1;
  for (auto it = locals.crbegin(); it != locals.crend(); it++) {
    if (it->name == name) {
      if (!it->ready) {
        error_at(previous, "Can't read local variable in its own initializer.",
                 "[Resolving error]");
      }
      return {static_cast<uint8_t>(idx), true};
    }
    idx--;
  }
  return {0, false};
}

std::pair<uint8_t, bool> Compiler::resolve_upvalue(const std::string& name) {
  // Recursively resolves name in enclosing lexicical scopes.
  if (enclosing == nullptr) {
    return {0, false};
  }
  auto [idx_if_found, found] = enclosing->resolve_local(name);
  if (found) {
    enclosing->locals[idx_if_found].is_captured = true;
    return {add_or_get_upvalue(idx_if_found, true), true};
  }
  std::tie(idx_if_found, found) = enclosing->resolve_upvalue(name);
  if (found) {
    return {add_or_get_upvalue(idx_if_found, false), true};
  }
  return {0, false};
}

uint8_t Compiler::add_or_get_upvalue(uint8_t index, bool is_local) {
  // Returns existing upvalue's index if the same variable from enclosing function
  // is referenced multiple times.
  uint8_t upvalues_idx = 0;
  // uint8_t as upvalues.size() as the check below guarantees the size for correct programs.
  for (auto it = upvalues.cbegin(); it != upvalues.cend(); it++, upvalues_idx++) {
    if (it->index == index && it->is_local == is_local) {
      return upvalues_idx;
    }
  }
  upvalues.push_back({.index=index, .is_local=is_local});
  size_t idx = upvalues.size() - 1;
  if (idx > std::numeric_limits<uint8_t>::max()) {
    error_at(previous, "Too many closure variables in function.");
  }
  function->upvalue_count++;
  return static_cast<uint8_t>(idx);
}

uint8_t Compiler::add_constant(Value val) {
  size_t idx = function->chunk->add_constant(val);
  if (idx > std::numeric_limits<uint8_t>::max()) {
    error_at(previous, "Too many constants in code chunk. OP_CONSTANT uses a single byte operand.");
    // VM limitation: the instructions working with constants refer to them by a 1 byte slot index.
    // Exceeding 256 variables (max representable id is 255) would break addressing.
  }
  return static_cast<uint8_t>(idx);
}

uint16_t Compiler::emit_jump(const OpCode op) const {
  emit_opcode(op);
  emit_operand(0);
  emit_operand(0);
  return function->chunk->code.size() - 3;
  // Return's jump instruction's address for later backpatching.
}

void Compiler::emit_loop(size_t loop_start_instr_idx) {
  emit_opcode(OpCode::OP_LOOP);
  size_t jump_dist = function->chunk->code.size() + 2 - loop_start_instr_idx;
  // Intent: jump such that execution is resumed at "while" condition
  // evaluation.
  if (jump_dist > std::numeric_limits<uint16_t>::max()) {
    error_at(previous, "Loop body too large, too much code to jump over.");
  }
  function->chunk->code.push_back((jump_dist >> 8) & 0xff);
  function->chunk->code.push_back(jump_dist & 0xff);
}

void Compiler::patch_jump(uint16_t jump_instr_idx) {
  size_t jump_dist = function->chunk->code.size() - 3 - jump_instr_idx;
  // Intent: jump such that execution is resumed at code.size() (so 1 after last
  // instruction). While interpreting this jump instruction, the VM will consume
  // jump's 2 operands moving IP by 2 and then start processing the next
  // instruction moving IP by 1 and jump distance has to be adjusted.
  if (jump_dist > std::numeric_limits<uint16_t>::max()) {
    error_at_current("Too much code to jump over.");
  }
  function->chunk->code[jump_instr_idx + 1] = (jump_dist >> 8) & 0xff;
  function->chunk->code[jump_instr_idx + 2] = jump_dist & 0xff;
}

void Compiler::emit_operand(uint8_t byte) const {
  function->chunk->add_byte(byte, tokens[previous].get_line());
}

void Compiler::emit_opcode(const OpCode op) const {
  function->chunk->add_opcode(op, tokens[previous].get_line());
}

void Compiler::emit_opcodes(const OpCode op_one, const OpCode op_two) const {
  emit_opcode(op_one);
  emit_opcode(op_two);
}

void Compiler::emit_closure(Value val) {
  emit_opcode(OpCode::OP_CLOSURE);
  emit_operand(add_constant(val));
}

void Compiler::emit_constant(Value val) {
  // Constants stay alive as long as the owning function is reachable during GC.
  emit_opcode(OpCode::OP_CONSTANT);
  emit_operand(add_constant(val));
}

void Compiler::emit_return() const {
  emit_opcodes(OpCode::OP_NIL, OpCode::OP_RETURN);
}

void Compiler::end_compiler() const {
  emit_return();
  heap->deregister_root_marking_callback();
  // TODO: Move to ctor & dctor
#ifdef DEBUG_PRINT_CODE
  if (!had_error) {
    disassembler.disassemble_chunk(*function->chunk, *function->name);
  }
#endif
}

bool Compiler::check(TokenType ttype) const {
  return tokens[current].get_type() == ttype;
}

void Compiler::consume(TokenType ttype, const std::string& err_msg) {
  if (check(ttype)) {
    advance();
  } else {
    error_at_current(err_msg);
  }
}

void Compiler::synchronize() {
  // Skips tokens until familiar territory is found - either end  (";") or
  // beginning of a statement (all the cases in switch).
  panic_mode = false;
  while (tokens[current].get_type() != TokenType::LOX_EOF) {
    if (tokens[previous].get_type() == TokenType::SEMICOLON) return;
    switch (tokens[current].get_type()) {
      case TokenType::CLASS:
      case TokenType::FUN:
      case TokenType::VAR:
      case TokenType::FOR:
      case TokenType::IF:
      case TokenType::WHILE:
      case TokenType::PRINT:
      case TokenType::RETURN:
        return;
      default:;  // noop
    }
    advance();
  }
}

void Compiler::error_at_current(const std::string& err_msg) {
  error_at(current, err_msg);
}

void Compiler::error_at(const size_t idx, const std::string& err_msg,
                        const std::string& stage) {
  if (panic_mode) return;
  // Invalid syntax detected previously and parser is lost in the grammar - do
  // not print a cascade of errors.
  panic_mode = true;
  had_error = true;

  Token token{tokens.at(idx)};
  std::string suffix;
  if (token.get_type() == TokenType::LOX_EOF) {
    suffix = "at end: " + err_msg;
  } else {
    suffix = "at '" + token.get_lexeme() + "'. " + err_msg;
  }
  std::string full_msg{stage + " [line " + std::to_string(token.get_line()) +
                       "] error: bad syntax while parsing: Got TokenType::" +
                       Token::token_type_string(token.get_type()) +
                       " with lexeme: '" + token.get_lexeme() + "'. " +
                       err_msg};
  e_reporter.set_error(full_msg);
}

bool Compiler::match(TokenType ttype) {
  if (tokens[current].get_type() == ttype) {
    advance();
    return true;
  }
  return false;
}
void Compiler::register_gc_callbacks() const {
  heap->register_root_marking_callback([this]{
  #ifdef DEBUG_LOG_GC
    std::cout << "[Compiler] Marking roots" << std::endl;
  #endif

    heap->mark(function);
  
  #ifdef DEBUG_LOG_GC
    std::cout << "[Compiler] Done marking roots" << std::endl;
  #endif
  });
}

}; //namespace cpplox
