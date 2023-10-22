#include "cpplox/Bytecode/Chunk.h"

#include <limits>

namespace chunk {

template <typename Enumeration>
auto constexpr as_uint8t(Enumeration const value) ->
    typename std::underlying_type<Enumeration>::type {
  return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

void Chunk::add_byte(const uint8_t byte, int lineno) {
  code.push_back(byte);
  line_numbers.push_back(lineno);
};

void Chunk::add_opcode(const OpCode opcode, int lineno) {
  this->add_byte(as_uint8t(opcode), lineno);
};

uint8_t Chunk::add_constant(Value val) {
  constants.push_back(val);
  if (constants.size() - 1 >= std::numeric_limits<uint8_t>::max()) {
    throw std::length_error(
        "Too many constants in code chunk as OP_CONSTANT uses a single byte "
        "operand.");
  }
  return static_cast<uint8_t>(constants.size() - 1);
}

}  // namespace chunk