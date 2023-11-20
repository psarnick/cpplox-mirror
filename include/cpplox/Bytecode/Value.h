#pragma once
#include <exception>
#include <memory>
#include <string>
#include <variant>

namespace chunk {
class Chunk;
}  // namespace chunk

namespace value {

// Functions are the bridge between the compile time and runtime environments.
// They are created during compile time and invoked during runtime.
class Function {
 public:
  explicit Function(int arity, std::string name);
  int arity{0};
  std::string name{};
  std::unique_ptr<chunk::Chunk> chunk;
  // Lox program is broken into Functions and each Function owns its bytecode
  // Chunk.
  // TODO: This will never be nullptr - can this be reference or value?
};


struct Value;
typedef Value (*NativeFn)(int argCount, Value* args);
struct Value : public std::variant<double, bool, std::string, std::monostate, std::shared_ptr<Function>, NativeFn> {
  using variant::variant;
};
//using Value = std::variant<double, bool, std::string, std::monostate, std::shared_ptr<Function>>;

// Design smell: Variant cannot hold references & the way Values are used makes
// Value incompatible with std::unique_ptr. shared_ptr for now, but refactor
// into OOP.
// TODO: Consider encapsulating this as a class with private members & public
// const functions only
//       Reason: STL containers cannot store const objects, which forces VM to
//       use std::unordered_map<std::string, Value> globals instead of
//       std::unordered_map<std::string, const Value>.
std::string to_string(const Value& val);

/*
time ./cpplox_binary ../test/benchmark/equality.lox
  Pure std::variant (no NativeFn, no optimisation)
    2.60s user 4.21s system 99% cpu 6.841 total
    2.60s user 4.27s system 99% cpu 6.891 total

  Pure std::variant (no NativeFn, -O3)
    1.33s user 4.27s system 96% cpu 5.831 total
    1.32s user 4.13s system 99% cpu 5.466 total
    1.33s user 4.15s system 99% cpu 5.492 total
    1.35s user 4.14s system 99% cpu 5.503 total
    1.32s user 4.12s system 99% cpu 5.463 total

  struct Variant : public std::variant ... (with NativeFn, -O3)
    1.32s user 4.14s system 93% cpu 5.820 total
    1.32s user 4.11s system 99% cpu 5.445 total
    1.32s user 4.12s system 99% cpu 5.453 total
    1.32s user 4.29s system 99% cpu 5.626 total
    1.33s user 4.12s system 99% cpu 5.469 total

  struct Variant : public std::variant ... (with NativeFn, no optimisation)
    2.51s user 4.10s system 97% cpu 6.809 total
    2.52s user 4.11s system 99% cpu 6.649 total
    2.51s user 4.09s system 99% cpu 6.618 total
    2.52s user 4.44s system 99% cpu 6.986 total
*/
}  // namespace value