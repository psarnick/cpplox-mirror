#include "cpplox/Treewalk/Literal.h"

#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>

namespace clox::Types {
std::string literal_to_string(const Literal& literal) {
  using namespace std;
  if (holds_alternative<bool>(literal)) {
    return get<bool>(literal) ? "true" : "false";
  }

  if (holds_alternative<double>(literal)) {
    stringstream stream;
    stream << get<double>(literal);
    return stream.str();
  }

  if (holds_alternative<string>(literal)) {
    return get<string>(literal);
  }

  if (holds_alternative<monostate>(literal)) {
    return "nil";
  }

  throw runtime_error("non-exhaustive visitor");
}
}  // namespace clox::Types