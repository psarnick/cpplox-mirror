#include "cpplox/Bytecode/Compiler.h"

#include <stdexcept>

#include "cpplox/Bytecode/Debug.h"
#include "cpplox/Bytecode/common.h"
#include "cpplox/Treewalk/Scanner.h"
// TOOD: Move scanner to common / scanner package

std::unique_ptr<Chunk> Compiler::compile() {
  if (already_called) {
    throw std::logic_error(
        "Compiler not designed to be called multiple times, create a new "
        "instance.");
  }
  already_called = true;
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
  return std::move(chunk);
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

void Compiler::statement() {
  // Separate rule for statements allowed inside control flow body.
  // See: https://craftinginterpreters.com/global-variables.html#statements
  if (match(TokenType::PRINT)) {
    print_stmt();
  } else if (match(TokenType::LEFT_BRACE)) {
    begin_scope();
    block();
    end_scope();
  } else {
    expression_stmt();
  }
}

void Compiler::expression_stmt() {
  // Expressions have a stack effect of +1 (they produce a value on the stack),
  // but statements leave stack unchanged. As such, expression statement
  // evaluates to the underlying expression and discards the result (OP_POP at
  // the end).
  expression();
  consume(TokenType::SEMICOLON, "Expecting ; after expression.");
  emit_opcode(OpCode::OP_POP);
}

void Compiler::print_stmt() {
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
  emit_constant(lexeme.substr(1, lexeme.length() - 2));  // skip "
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
      throw std::logic_error("Unknown op in unary: " + token.to_string());
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

void Compiler::variable(const bool precedence_context_allows_assignment) {
  named_variable(tokens[previous].get_lexeme(),
                 precedence_context_allows_assignment);
}

void Compiler::parse_precedence(const Precedence& precedence) {
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
  // TODO: Test "var 5 = 2;" or "5 = 2";
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
    return emit_identifier_constant(tokens[previous].get_lexeme());
    // Globals are resolved dynamically - write var name to constants table &
    // return its index
  } else {
    return 0;  // placeholder
  }
}

void Compiler::declare_variable() {
  if (scope_depth == 0) return;
  // globals are lazy-bound, no need to capture declarations

  std::string var_name = tokens[previous].get_lexeme();
  for (auto it = locals.crbegin(); it != locals.crend(); it++) {
    // TODO: does this need to look at ready?
    if (it->depth < scope_depth) {
      break;  // already in parent scope
    }
    if (it->name == var_name) {
      error_at(previous, "Variable with this name already in scope.");
    }
    /* Detect situations like
        {
            var a = 1;
            var a = 2;
        }
    */
  }

  if (locals.size() > std::numeric_limits<uint8_t>::max()) {
    error_at(previous, "Too many local variables.");
    // instruction operands are 8 bit values, exceeding 256 variables (max idx
    // 255) would break addressing.
    return;
  }
  locals.push_back({.name = var_name, .depth = -1, .ready = false});
}

void Compiler::define_variable(
    const uint8_t const_table_index_of_global_variable_name) {
  if (scope_depth > 0) {
    locals.back().depth = scope_depth;
    locals.back().ready = true;
    return;  // bytecode not needed for local var - it's already on top of stack
  } else {
    // Global variables are looked up by name at runtime, but entire variable
    // name is too large to store as bytecode operand - store it in constants
    // table and make bytecode refer to the global var by index instead.
    // TODO: clox's reference runtime implementation looks up globals by string
    // name
    //       But it also already identifies them by array index (here), so it
    //       should be possible to use vector instead of map (unless problems in
    //       later chapters).
    emit_opcode(OpCode::OP_DEFINE_GLOBAL);
    emit_operand(const_table_index_of_global_variable_name);
  }
}

void Compiler::begin_scope() { scope_depth++; }

void Compiler::end_scope() {
  scope_depth--;
  int cnt = 0;
  for (auto it = locals.crbegin(); it != locals.crend(); it++) {
    if (it->depth == scope_depth) {
      break;
    }
    emit_opcode(OpCode::OP_POP);
    cnt++;
  }
  locals.erase(locals.end() - cnt, locals.end());
}

void Compiler::named_variable(const std::string& var_name,
                              const bool precedence_context_allows_assignment) {
  auto [idx_if_found, found] = resolve_local(var_name);
  OpCode get_op;  // Op to emit if named variable is read from.
  OpCode set_op;  // Op to emit if named variable is written to.
  uint8_t idx{0};
  if (found) {
    idx = idx_if_found;
    get_op = OpCode::OP_GET_LOCAL;
    set_op = OpCode::OP_SET_LOCAL;
  } else {
    idx = emit_identifier_constant(var_name);
    // TODO_NOW: This always adds gloabl variables to constants pool, even when
    // they are read.
    // TODO_NOW: Challenge at
    // https://craftinginterpreters.com/global-variables.html#reading-variables
    // Store global variable name in chunk's constants pool.
    get_op = OpCode::OP_GET_GLOBAL;
    set_op = OpCode::OP_SET_GLOBAL;
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
  size_t idx = locals.size() - 1;
  for (auto it = locals.crbegin(); it != locals.crend(); it++) {
    if (it->name == name) {
      assert(idx <= std::numeric_limits<uint8_t>::max());
      if (!it->ready) {
        error_at(previous, "Can't read local variable in its own initializer.");
      }
      return {(uint8_t)idx, true};
    }
    idx--;
  }
  return {0, false};
}

uint8_t Compiler::emit_identifier_constant(const Value& val) const {
  return chunk->add_constant(val);
}

void Compiler::emit_operand(uint8_t byte) const {
  chunk->add_byte(byte, tokens[previous].get_line());
}

void Compiler::emit_opcode(const OpCode byte) const {
  chunk->add_opcode(byte, tokens[previous].get_line());
}

void Compiler::emit_opcodes(const OpCode first, const OpCode second) const {
  emit_opcode(first);
  emit_opcode(second);
}

void Compiler::emit_constant(const Value& val) const {
  emit_opcode(OpCode::OP_CONSTANT);
  emit_operand(chunk->add_constant(val));
}

void Compiler::emit_return() const { emit_opcode(OpCode::OP_RETURN); }

void Compiler::end_compiler() const {
  emit_return();
#ifdef DEBUG_PRINT_CODE
  if (!had_error) {
    disassembler.disassemble_chunk(*chunk, "code");
  }
#endif  // DEBUG_PRINT_CODE
}

bool Compiler::check(const TokenType& ttype) const {
  return tokens[current].get_type() == ttype;
}

void Compiler::consume(const TokenType& ttype, const std::string& err_msg) {
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

void Compiler::error_at(const size_t idx, const std::string& err_msg) {
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
  std::string full_msg{
      "[Parsing error] [line " + std::to_string(token.get_line()) +
      "] error: bad syntax while parsing: Got TokenType::" +
      Token::token_type_string(token.get_type()) + " with lexeme: '" +
      token.get_lexeme() + "'. " + err_msg};
  e_reporter.set_error(full_msg);
}

bool Compiler::match(const TokenType& ttype) {
  if (tokens[current].get_type() == ttype) {
    advance();
    return true;
  }
  return false;
}
