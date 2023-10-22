#pragma once
#include <iostream>
#include <optional>
#include <string>
#include <variant>

namespace clox::Types {
using Literal = std::variant<bool, double, std::string, std::monostate>;
std::string literal_to_string(const Literal& literal);
}  // namespace clox::Types