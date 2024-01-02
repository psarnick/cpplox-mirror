#pragma once
#include <cstdint>
#include <vector>

#include "Value.h"

namespace cpplox {

  enum class OpCode : uint8_t {
    OP_RETURN,         // [opcode]
    OP_CONSTANT,       // [opcode, constant's index]
    OP_NIL,            // [opcode]
    OP_TRUE,           // [opcode]
    OP_FALSE,          // [opcode]
    OP_POP,            // [opcode]
    OP_GET_LOCAL,      // [opcode, local's stack index]
    OP_SET_LOCAL,      // [opcode, local's stack index]
    OP_GET_GLOBAL,     // [opcode, global's constant index]
    OP_DEFINE_GLOBAL,  // [opcode, global's constant index]
    OP_SET_GLOBAL,     // [opcode, global's constant index]
    OP_EQUAL,          // [opcode] and 2 values taken from stack
    OP_GREATER,        // [opcode] and 2 values taken from stack
    OP_LESS,           // [opcode] and 2 values taken from stack
    OP_ADD,            // [opcode] and 2 values taken from stack
    OP_SUBTRACT,       // [opcode] and 2 values taken from stack
    OP_MULTIPLY,       // [opcode] and 2 values taken from stack
    OP_DIVIDE,         // [opcode] and 2 values taken from stack
    OP_NOT,            // [opcode] and 1 value  taken from stack
    OP_NEGATE,         // [opcode] and 1 value  taken from stack
    OP_PRINT,          // [opcode] and 1 value  taken from stack
    OP_JUMP_IF_FALSE,  // [opcode, offset's upper byte, offset's lower byte]
    OP_JUMP,           // [opcode, offset's upper byte, offset's lower byte]
    OP_LOOP,           // [opcode, offset's upper byte, offset's lower byte]
    OP_CALL,           // [opcode, number of func call arguments]
    OP_CLOSURE,        // [opcode, function's constant index, 2 bytes per upvalue]
    OP_NOOP,           // [opcode, operand]
    OP_GET_UPVALUE,    // [opcode, upvalue's index in current closure]
    OP_SET_UPVALUE,    // [opcode, upvalue's index in current closure]
    OP_CLOSE_UPVALUE,  // [opcode]
  };

  class Chunk {
  public:
    explicit Chunk(){};
    void add_byte(const uint8_t byte, const int lineno);
    void add_opcode(const OpCode opcode, const int lineno);
    size_t add_constant(Value val);
    // Returns added value's index in constants vector and does not enforce that 
    // at most uint8::max values can be stored.

    std::vector<uint8_t> code;
    // This implementation stores instructions (OpCode type) and operand indices
    // (uint8_t) in a single vector, treating both as bytes. Consumers are
    // expected to know when casting uint8_t to OpCode is needed during reading.
    std::vector<int> line_numbers;
    // TODO optimisation: as of now, each call to add_byte adds entry to
    // line_numbers. Consider run-length encoding to have memory.
    std::vector<Value> constants;
  };

}  // namespace cpplox