#include "cpplox/Bytecode/Value.h"

#include <sstream>

#include "cpplox/Bytecode/Chunk.h"
// TODO: Does this duplicate code from Treewalk's Value?

namespace value {

Function::Function(int arity, std::string name)
    : arity{arity}, name{name}, chunk{std::make_unique<chunk::Chunk>()} {};

std::string to_string(const Value& val) {
  // TODO: Code duplicated from Literal.cpp - how to refactor into one?
  using namespace std;
  if (holds_alternative<bool>(val)) {
    return get<bool>(val) ? "true" : "false";
  }
  if (holds_alternative<double>(val)) {
    stringstream stream;
    stream << std::get<double>(val);
    return stream.str();
    // return std::to_string(std::get<double>(val));
  }
  if (holds_alternative<string>(val)) {
    return get<string>(val);
  }
  if (holds_alternative<monostate>(val)) {
    return "nil";
  }
  if (holds_alternative<shared_ptr<Function>>(val)) {
    return "<fn " + get<shared_ptr<Function>>(val)->name + ">";
  }
  throw std::logic_error("non-exhaustive visitor");
}

}  // namespace value
