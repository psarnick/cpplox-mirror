#pragma once
#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <list>
#include <optional>

#include "Debug.h"
#include "cpplox/Treewalk/ErrorReporter.h"
#include "cpplox/Treewalk/Token.h"
#include "cpplox/Bytecode/GC.h"
#include "cpplox/Bytecode/LoxObject.h"
#include "cpplox/Bytecode/StringPool.h"

using namespace clox::Types;
using clox::ErrorsAndDebug::ErrorReporter;

namespace cpplox {
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
      // TODO optimisation: tokens live as long as locals, so this could be a string_view.
      int depth{0};
      // Level of nesting this local lives in, 1 = first top-level block, 2 =
      // inside of 1 and so on. Used to track which block each local variable
      // belongs to.
      bool ready{false};
      // Marks whether variable has been fully initialized and is ready for use.
      // Done to correctly handle: { var a = "outer"; { var a = a; } }
      bool is_captured{false};
    };

    struct CompiletimeUpvalue {
      uint8_t index {0};
      //  Closed-over variables's:
      //  is_local is true:  stack index relative to enclosing function's stack position.
      //  is_local is false: upvalue index in enclosing function's upvalue collection.
      bool is_local {false};
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
                          .infix = &Compiler::call,
                          .precedence = Precedence::CALL},
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
        {.prefix = nullptr,
        .infix = &Compiler::and_,
        .precedence = Precedence::AND},
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
        {.prefix = nullptr,
        .infix = &Compiler::or_,
        .precedence = Precedence::OR},
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
                      const Disassembler& disassembler, ErrorReporter& e_reporter,
                      gc_heap* const heap, StringPool* const pool,
                      size_t token_idx = 0, Compiler* const enclosing = nullptr)
        : tokens{tokens},
          disassembler{disassembler},
          e_reporter{e_reporter},
          heap{heap},
          pool{pool},
          enclosing{enclosing},
          function{heap->make<Function>(0, 0, pool->insert_or_get("script"), std::make_unique<Chunk>())},
          locals{},
          healthy{true} {
      if (token_idx > 0) {
        current = token_idx;
        previous = token_idx - 1;
        function->name = pool->insert_or_get(tokens[previous].get_lexeme());
      }
      locals.push_back({.name = "", .depth = 0, .ready = false, .is_captured = false});
      // locals is used to calculate stack window offsets for local variables.
      // However, for each function call, VM reserves one stack slot for internal
      // purposes. This unnnamed local entry is added to adjust calculated
      // indices.
    };
    std::optional<function_ptr> compile();
    // Compile can only be called once, very meh but simpler for now.

  private:
    const std::vector<const Token>& tokens;
    const Disassembler& disassembler;
    ErrorReporter& e_reporter;
    gc_heap* const heap{nullptr};
    StringPool* const pool{nullptr};
    Compiler* const enclosing{nullptr};
    function_ptr function;
    std::vector<Local> locals;
    // All locals that are in scope during each point of compilation, ordered by
    // order of declaration in code. Note: OP_GET_LOCAL and OP_SET_LOCAL use 1
    // byte operands, so locals.size() can be 256 at most.
    std::list<CompiletimeUpvalue> upvalues;
    // Resolved identifiers that this function closes-over, ordered declaration
    // from earliest to last. Just like locals, upvalues.size() can be 256 at most.
    size_t current{0};
    size_t previous{0};
    bool had_error{false};
    bool panic_mode{false};
    bool healthy{true};
    int scope_depth{0};
    // 0 = global scope, 1 = 1st to-level block, 2 = inside of 1st, ...

    void advance();
    void declaration();
    void dispatch_function_declaration();
    void function_declaration();
    void var_declaration();
    void statement();
    void expression_statement();
    void print_statement();
    void block();
    void expression();
    void number(const bool precedence_context_allows_assignment);
    void string(const bool precedence_context_allows_assignment);
    void grouping(bool precedence_context_allows_assignment);
    void unary(const bool precedence_context_allows_assignment);
    void binary(const bool precedence_context_allows_assignment);
    void and_(const bool precedence_context_allows_assignment);
    void or_(const bool precedence_context_allows_assignment);
    void call(const bool precedence_context_allows_assignment);
    void literal(const bool precedence_context_allows_assignment);
    void variable(const bool precedence_context_allows_assignment);
    void parse_precedence(const Precedence& precedence);
    uint8_t parse_variable(const std::string& err_msg);
    void declare_variable();
    void define_variable(const uint8_t const_table_index_of_global_variable_name);
    void begin_scope();
    void end_scope();
    void if_statement();
    void while_statement();
    void for_statement();
    void return_statement();
    void named_variable(const std::string var_name,
                        const bool precedence_context_allows_assignment);
    std::pair<uint8_t, bool> resolve_local(const std::string& name);
    std::pair<uint8_t, bool> resolve_upvalue(const std::string& name);
    // TODO: Refactor into string_views?
    uint8_t add_or_get_upvalue(uint8_t idx, bool is_local);
    uint8_t add_constant(Value val);
    uint16_t emit_jump(const OpCode op) const;
    void emit_loop(size_t loop_start_instr_idx);
    void patch_jump(uint16_t jump_instr_idx);
    void emit_operand(const uint8_t byte) const;
    void emit_opcode(const OpCode op) const;
    void emit_opcodes(const OpCode op_one, const OpCode op_two) const;
    void emit_constant(Value val);
    void emit_closure(Value val);
    void emit_return() const;
    void end_compiler() const;
    bool check(TokenType ttype) const;
    void consume(TokenType ttype, const std::string& err_msg);
    void synchronize();
    void error_at_current(const std::string& err_msg);
    void error_at(const size_t idx, const std::string& err_msg,
                  const std::string& stage = "[Parsing error]");
    bool match(TokenType ttype);
    void register_gc_callbacks() const;
  };

}; // namespace cpplox