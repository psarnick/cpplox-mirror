#pragma once
#include <cstdint>
#include <vector>

#include "Value.h"

namespace chunk {

using namespace value;

enum class OpCode : uint8_t {
  OP_RETURN,    // [opcode]
  OP_CONSTANT,  // [opcode, constant's index]
  OP_NIL,       // [opcode]
  OP_TRUE,      // [opcode]
  OP_FALSE,     // [opcode]
  OP_POP,       // [opcode]
  OP_GET_LOCAL,
  OP_SET_LOCAL,
  OP_GET_GLOBAL,
  OP_DEFINE_GLOBAL,  // [opcode, constant's index]
  OP_SET_GLOBAL,     // [opcode, constant's index]
  OP_EQUAL,          // [opcode] and 2 values taken from stack
  OP_GREATER,        // [opcode] and 2 values taken from stack
  OP_LESS,           // [opcode] and 2 values taken from stack
  OP_ADD,            // [opcode] and 2 values taken from stack
  OP_SUBTRACT,       // [opcode] and 2 values taken from stack
  OP_MULTIPLY,       // [opcode] and 2 values taken from stack
  OP_DIVIDE,         // [opcode] and 2 values taken from stack
  OP_NOT,            // [opcode] and 1 value  taken from stack
  OP_NEGATE,         // [opcode] and 1 value  taken from stack
  OP_PRINT           // [opcode] and 1 value  taken from stack
};

class Chunk {
 public:
  explicit Chunk(){};
  void add_byte(const uint8_t byte, const int lineno);
  void add_opcode(const OpCode opcode, const int lineno);
  uint8_t add_constant(Value val);
  // Makes a copy of every Value to decouple lifetimes, for this reason should
  // not accept pointers or references once string interning is added. Returns
  // added value's index in constants vector. Throws if uint8 size limit is
  // exceeded.

  std::vector<uint8_t> code;
  // This implementation stores instructions (OpCode type) and operand indices
  // (uint8_t) in a single vector, treating both as bytes. Consumers are
  // expected to know when casting uint8_t to OpCode is needed during reading.
  std::vector<int> line_numbers;
  // Optimisation: as of now, each call to add_byte adds entry to line_numbers.
  // Consider run-length encoding to have memory.
  std::vector<Value> constants;
};

}  // namespace chunk