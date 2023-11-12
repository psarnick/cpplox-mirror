#include "cpplox/Bytecode/Debug.h"

#include <iomanip>

#include "stdio.h"

namespace debug {
using namespace value;

void Disassembler::disassemble_chunk(const Chunk& chunk,
                                     const std::string name) const {
  debug_out << "=== compiled chunk " << name << " === " << std::endl;
  size_t offset{0};
  while (offset < chunk.code.size()) {
    offset = disassemble_instruction(chunk, offset);
  }
  debug_out << "==/ compiled chunk " << name << " /== " << std::endl;
}

size_t Disassembler::disassemble_instruction(const Chunk& chunk,
                                             int offset) const {
  debug_out << std::setfill('0') << std::setw(4) << std::right << offset << " ";
  if (offset > 0 &&
      chunk.line_numbers[offset - 1] == chunk.line_numbers[offset]) {
    debug_out << "   | ";
  } else {
    debug_out << std::setfill(' ') << std::setw(4) << chunk.line_numbers[offset]
              << " ";
  }

  OpCode instruction{chunk.code.at(offset)};
  switch (instruction) {
    case OpCode::OP_RETURN:
      return simple_instruction("OP_RETURN", offset);
    case OpCode::OP_CONSTANT:
      return constant_instruction("OP_CONSTANT", chunk, offset);
    case OpCode::OP_NIL:
      return simple_instruction("OP_NIL", offset);
    case OpCode::OP_TRUE:
      return simple_instruction("OP_TRUE", offset);
    case OpCode::OP_FALSE:
      return simple_instruction("OP_FALSE", offset);
    case OpCode::OP_POP:
      return simple_instruction("OP_POP", offset);
    case OpCode::OP_GET_LOCAL:
      return byte_instruction("OP_GET_LOCAL", chunk, offset);
    case OpCode::OP_SET_LOCAL:
      return byte_instruction("OP_SET_LOCAL", chunk, offset);
    case OpCode::OP_GET_GLOBAL:
      return constant_instruction("OP_GET_GLOBAL", chunk, offset);
    case OpCode::OP_DEFINE_GLOBAL:
      return constant_instruction("OP_DEFINE_GLOBAL", chunk, offset);
    case OpCode::OP_SET_GLOBAL:
      return constant_instruction("OP_SET_GLOBAL", chunk, offset);
    case OpCode::OP_EQUAL:
      return simple_instruction("OP_EQUAL", offset);
    case OpCode::OP_GREATER:
      return simple_instruction("OP_GREATER", offset);
    case OpCode::OP_LESS:
      return simple_instruction("OP_LESS", offset);
    case OpCode::OP_ADD:
      return simple_instruction("OP_ADD", offset);
    case OpCode::OP_SUBTRACT:
      return simple_instruction("OP_SUBTRACT", offset);
    case OpCode::OP_MULTIPLY:
      return simple_instruction("OP_MULTIPLY", offset);
    case OpCode::OP_DIVIDE:
      return simple_instruction("OP_DIVIDE", offset);
    case OpCode::OP_NOT:
      return simple_instruction("OP_NOT", offset);
    case OpCode::OP_NEGATE:
      return simple_instruction("OP_NEGATE", offset);
    case OpCode::OP_PRINT:
      return simple_instruction("OP_PRINT", offset);
    case OpCode::OP_JUMP:
      return jump_instruction("OP_JUMP", 1, chunk, offset);
    case OpCode::OP_JUMP_IF_FALSE:
      return jump_instruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
    case OpCode::OP_LOOP:
      return jump_instruction("OP_LOOP", -1, chunk, offset);
    default:
      debug_out << "Unknown opcode: " << static_cast<unsigned int>(instruction)
                << std::endl;
      return offset + 1;
  }
}

void Disassembler::disassemble_constants_table(const Chunk& chunk) const {
  debug_out << "=== constants ===";
  for (size_t i = 0; i < chunk.constants.size(); i++) {
    debug_out << std::endl
              << "    " << i << "    " << to_string(chunk.constants[i]);
  }
  debug_out << std::endl << "==/ constants /==" << std::endl;
}

size_t Disassembler::simple_instruction(const std::string name,
                                        size_t offset) const {
  debug_out << name << std::endl;
  return offset + 1;
}

size_t Disassembler::constant_instruction(const std::string name,
                                          const Chunk& chunk,
                                          size_t offset) const {
  uint8_t const_idx = chunk.code[offset + 1];
  debug_out << std::setfill(' ') << std::left << name << std::setw(8) << " ";
  debug_out << static_cast<unsigned int>(const_idx) << " ";
  debug_out << "'" << to_string(chunk.constants[const_idx]) << "'" << std::endl;
  return offset + 2;
}

size_t Disassembler::byte_instruction(const std::string name,
                                      const Chunk& chunk, size_t offset) const {
  uint8_t idx = chunk.code[offset + 1];
  debug_out << std::setfill(' ') << std::left << name << std::setw(8) << " ";
  debug_out << static_cast<unsigned int>(idx) << " " << std::endl;
  return offset + 2;
}

size_t Disassembler::jump_instruction(const std::string name, int sign, const Chunk& chunk, size_t offset) const {
  uint16_t jump = static_cast<uint16_t>((chunk.code[offset + 1] << 8));
  jump |= chunk.code[offset + 2];
  debug_out << std::setfill(' ') << std::left << name << std::setw(20 - name.length()) << std::right
    << offset << " -> " << static_cast<int>(offset) + 3 + sign * jump << std::endl;
  // TODO: Hacky for now and will break, make adjustments dynamic.
  return offset + 3;
}

}  // namespace debug
