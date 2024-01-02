#pragma once

#include <fstream>

#include "Chunk.h"

namespace cpplox {

  class Disassembler {
  public:
    // TODO: Refactor to use string_views.
    explicit Disassembler(std::ofstream& debug_out) : debug_out(debug_out){};
    void disassemble_chunk(const Chunk& chunk, const std::string name) const;
    size_t disassemble_instruction(const Chunk& chunk, int offset) const;
    void disassemble_constants_table(const Chunk& chunk, const std::string& name) const;

    // Invariant: functions below should return the address of the next
    // instruction
    size_t simple_instruction(const std::string name, size_t offset) const;
    size_t constant_instruction(const std::string name, const Chunk& chunk,
                                size_t offset) const;
    size_t byte_instruction(const std::string name, const Chunk& chunk,
                            size_t offset) const;
    size_t jump_instruction(const std::string name, int sign, const Chunk& chunk,
                            size_t offset) const;

  private:
    std::ofstream& debug_out;
    static const int max_line_len{30};
  };
}  // namespace cpplox
