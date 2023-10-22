#pragma once

#include <exception>
#include <string>
#include <variant>

namespace value {
using Value = std::variant<double, bool, std::string, std::monostate>;
// TODO: Consider encapsulating this as a class with private members & public
// const functions only
//       Reason: STL containers cannot store const objects, which forces VM to
//       use std::unordered_map<std::string, Value> globals instead of
//       std::unordered_map<std::string, const Value>.
std::string to_string(const Value& val);
}  // namespace value