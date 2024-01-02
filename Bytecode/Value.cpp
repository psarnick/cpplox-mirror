#include <sstream>

#include "cpplox/Bytecode/Value.h"
#include "cpplox/Bytecode/Chunk.h"
#include "cpplox/Bytecode/GC.h"
#include "cpplox/Bytecode/LoxObject.h"

namespace cpplox {

std::string to_string(const Value val) {
  // TODO: Code duplicated from Literal.cpp - how to refactor into one?
  // TODO: Replace with visitor or get_if
  using namespace std;
  if (holds_alternative<bool>(val)) {
    return get<bool>(val) ? "true" : "false";
  }
  if (holds_alternative<double>(val)) {
    stringstream stream;
    stream << std::get<double>(val);
    return stream.str();
  }
  if (holds_alternative<const_string_ptr>(val)) {
    return *get<const_string_ptr>(val);
  }
  if (holds_alternative<monostate>(val)) {
    return "nil";
  }
  if (holds_alternative<function_ptr>(val)) {
    return "<fn " + *get<function_ptr>(val)->name + ">";
  }
  if (holds_alternative<native_function_ptr>(val)) {
    return "<native fn>";
  }
  if (holds_alternative<closure_ptr>(val)) {
    return "<fn " + *get<closure_ptr>(val)->function->name + ">";
  }
  throw std::logic_error("Value: non-exhaustive visitor: " + std::to_string(val.index()));
}

}  // namespace cpplox