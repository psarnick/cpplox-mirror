#pragma once
#include <cassert>
#include <memory>
#include <string>
#include <utility>

#include "Chunk.h"
#include "Debug.h"
#include "cpplox/Treewalk/ErrorReporter.h"
#include "cpplox/Treewalk/Token.h"

using namespace chunk;
using namespace clox::Types;
using clox::ErrorsAndDebug::ErrorReporter;
using namespace debug;

// Compiler combines parsing and code generation into one step with no
// intermediary AST being produced. This limits the amount of syntax context
// available to compiler, but Lox's grammar is simple enough to allow that.

class Compiler {
  enum class Precedence {
    NONE,
    ASSIGNMENT,  // =
    OR,          // or
    AND,         // and
    EQUALITY,    // == !=
    COMPARISON,  // < > <= >=
    TERM,        // + -
    FACTOR,      // * /
    UNARY,       // ! -
    CALL,        // . ()
    PRIMARY
  };
  // Lowest to highest precedence.

  struct Local {
    std::string name{};
    int depth{0};
    bool ready{false};
  };

  typedef void (Compiler::*ParseFn)(const bool);
  // Some functions have side-effects on the Compiler object, so signature of a
  // non-const member function.
  struct ParseRule {
    const ParseFn prefix;
    const ParseFn infix;
    const Precedence precedence;
  };

  const std::vector<const ParseRule> rules = {
      /* LEFT_PAREN */ {.prefix = &Compiler::grouping,
                        .infix = nullptr,
                        .precedence = Precedence::NONE},
      /* RIGHT_PAREN */
      {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::NONE},
      /* LEFT_BRACE */
      {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::NONE},
      /* RIGHT_BRACE */
      {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::NONE},
      /* COMMA */
      {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::NONE},
      /* DOT */
      {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::NONE},
      /* MINUS */
      {.prefix = &Compiler::unary,
       .infix = &Compiler::binary,
       .precedence = Precedence::TERM},
      /* PLUS */
      {.prefix = nullptr,
       .infix = &Compiler::binary,
       .precedence = Precedence::TERM},
      /* SEMICOLON */
      {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::NONE},
      /* SLASH */
      {.prefix = nullptr,
       .infix = &Compiler::binary,
       .precedence = Precedence::FACTOR},
      /* STAR */
      {.prefix = nullptr,
       .infix = &Compiler::binary,
       .precedence = Precedence::FACTOR},
      /* BANG */
      {.prefix = &Compiler::unary,
       .infix = nullptr,
       .precedence = Precedence::NONE},
      /* BANG_EQUAL */
      {.prefix = nullptr,
       .infix = &Compiler::binary,
       .precedence = Precedence::EQUALITY},
      /* EQUAL */
      {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::NONE},
      /* EQUAL_EQUAL */
      {.prefix = nullptr,
       .infix = &Compiler::binary,
       .precedence = Precedence::EQUALITY},
      /* GREATER */
      {.prefix = nullptr,
       .infix = &Compiler::binary,
       .precedence = Precedence::COMPARISON},
      /* GREATER_EQUAL */
      {.prefix = nullptr,
       .infix = &Compiler::binary,
       .precedence = Precedence::COMPARISON},
      /* LESS */
      {.prefix = nullptr,
       .infix = &Compiler::binary,
       .precedence = Precedence::COMPARISON},
      /* LESS_EQUAL */
      {.prefix = nullptr,
       .infix = &Compiler::binary,
       .precedence = Precedence::COMPARISON},
      /* IDENTIFIER */
      {.prefix = &Compiler::variable,
       .infix = nullptr,
       .precedence = Precedence::NONE},
      /* STRING */
      {.prefix = &Compiler::string,
       .infix = nullptr,
       .precedence = Precedence::NONE},
      /* NUMBER */
      {.prefix = &Compiler::number,
       .infix = nullptr,
       .precedence = Precedence::NONE},
      /* AND */
      {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::NONE},
      /* CLASS */
      {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::NONE},
      /* ELSE */
      {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::NONE},
      /* FALSE */
      {.prefix = &Compiler::literal,
       .infix = nullptr,
       .precedence = Precedence::NONE},
      /* FUN */
      {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::NONE},
      /* FOR */
      {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::NONE},
      /* IF */
      {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::NONE},
      /* NIL */
      {.prefix = &Compiler::literal,
       .infix = nullptr,
       .precedence = Precedence::NONE},
      /* OR */
      {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::NONE},
      /* PRINT */
      {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::NONE},
      /* RETURN */
      {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::NONE},
      /* SUPER */
      {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::NONE},
      /* THIS */
      {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::NONE},
      /* TRUE */
      {.prefix = &Compiler::literal,
       .infix = nullptr,
       .precedence = Precedence::NONE},
      /* VAR */
      {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::NONE},
      /* WHILE */
      {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::NONE},
      /* LOX_EOF */
      {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::NONE},
  };
  // Each (syntactical context, token type) pair is mapped to an
  // expression-parsing function that emits appropriate bytecode. Vector indices
  // correspond to numerical value of TokenType, so order of fields in TokenType
  // class must agree with above vector. Prefix functions: compile expression
  // starting with a token of that type. Leading token was already consumed
  //     when this function is called. All prefix operators in Lox have the same
  //     precedence, so .precedence is not used for prefix expressions.
  // Infix functions:  compile expression whose left operand is followed by a
  // token of that type. .precedence
  //     is necessary for infix operators to correctly implement left/right
  //     associativity, for example 1 + 2 + 3 + 4 -> ((1 + 2) + 3) + 4 for
  //     left-associative. LHS and infix token were already consumed when this
  //     function is called.
  // Precedence: precedence of an infix expression that uses that token as an
  // operator.

 public:
  explicit Compiler(const std::vector<const Token>& tokens,
                    const Disassembler& disassembler, ErrorReporter& e_reporter)
      : tokens{tokens},
        disassembler{disassembler},
        e_reporter{e_reporter},
        chunk{std::make_unique<Chunk>()} {};
  std::unique_ptr<Chunk> compile();
  // Compile can only be called once, very meh but simpler for now.

 private:
  const std::vector<const Token>& tokens;
  const Disassembler& disassembler;
  ErrorReporter& e_reporter;
  std::unique_ptr<Chunk> chunk;
  size_t current{0};
  size_t previous{0};
  bool had_error{false};
  bool panic_mode{false};
  bool already_called{false};
  std::vector<Local> locals;
  int scope_depth{0};

  void advance();
  void declaration();
  void var_declaration();
  void statement();
  void expression_stmt();
  void print_stmt();
  void block();
  void expression();
  void number(const bool precedence_context_allows_assignment);
  void string(const bool precedence_context_allows_assignment);
  void grouping(bool precedence_context_allows_assignment);
  void unary(const bool precedence_context_allows_assignment);
  void binary(const bool precedence_context_allows_assignment);
  // Compiled bytecode order is such that VM first executes LHS, then RHS and
  // finally the instruction.
  void literal(const bool precedence_context_allows_assignment);
  void variable(const bool precedence_context_allows_assignment);
  void parse_precedence(const Precedence& precedence);
  // Start at current token and parse expressions with equal-or-higher
  // precedence as defined in Precedence enum.
  uint8_t parse_variable(const std::string& err_msg);
  void declare_variable();
  void define_variable(const uint8_t const_table_index_of_global_variable_name);
  void begin_scope();
  void end_scope();
  void named_variable(const std::string& var_name,
                      const bool precedence_context_allows_assignment);
  std::pair<uint8_t, bool> resolve_local(const std::string& name);
  uint8_t emit_identifier_constant(const Value& val) const;
  void emit_operand(const uint8_t byte) const;
  void emit_opcode(const OpCode byte) const;
  void emit_opcodes(const OpCode first, const OpCode second) const;
  void emit_constant(const Value& val) const;
  void emit_return() const;
  void end_compiler() const;
  bool check(const TokenType& ttype) const;
  void consume(const TokenType& ttype, const std::string& err_msg);
  void synchronize();
  void error_at_current(const std::string& err_msg);
  void error_at(const size_t idx, const std::string& err_msg);
  bool match(const TokenType& ttype);
};