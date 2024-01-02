#pragma once
#include <span>
#include <memory>

#include "cpplox/Bytecode/Chunk.h"
#include "cpplox/Bytecode/GC.h"
#include "cpplox/Bytecode/LoxObjectFwd.h"

namespace cpplox {

  class RuntimeUpvalue;
  using upvalue_ptr = gc_ptr<RuntimeUpvalue>;

  struct Function {
    // Functions are the bridge between the compile time and runtime environments.
    // They are created during compile time and invoked during runtime.
    explicit Function(int arity, int upvalue_count, const_string_ptr name, std::unique_ptr<Chunk> chunk) : arity{arity}, upvalue_count{upvalue_count}, name{name}, chunk{std::move(chunk)} {};
    int arity{0};
    int upvalue_count{0};
    const_string_ptr name{};
    std::unique_ptr<Chunk> chunk;
    // Lox program is broken into Functions and each Function owns its bytecode Chunk.
    // TODO: Can this be simplified by storing Chunk by value instead?
  };

  struct NativeFn {
    explicit NativeFn(Value (*func)(int, std::span<Value>)) : func(func) {}
    Value (*func)(int arg_count, std::span<Value> args);
    // TODO: remove arg_count, unless needed for guarding.
    // TODO: NativeFn could also capture function name and expected arity.
    // Consider modifying the interface to let native functions allocate on gc_heap,
    // otherwise only double and bools can be returned.
  };

  class RuntimeUpvalue {
  // TODO: This is doing if/else but with lots of indirection. Benchmark against the pointer tricks that clox does and refactor if bad.
  private:
    struct OpenLocator {
      std::vector<Value>& stack;
      const size_t index;
      // Open upvalue is still on stack and (stack, index) represent its location. Not using span it can be invalidated
      // on resizes etc.
    };
    using ClosedLocator = Value;
    // Represents non-owning pointer type. Reason: RuntimeUpvalues close over variables, not values. Changes made to any variable
    // via RuntimeUpvalue must be visible to other RuntimeUpvalues that closed over the same variable. Closures, and their RuntimeUpvalues,
    // may be discarded in arbitrary order, so there's no single owner of value. What is more, variables captured in closures can live
    // beyond the closure itself (eg. by being stored in object field).
    std::variant<OpenLocator, ClosedLocator> val;

  public:
    explicit RuntimeUpvalue(std::vector<Value>& stack, size_t index) : val{OpenLocator{stack, index}} {};
    Value* value();
    // TODO: Add const override for read-only access.
    void close() { val = ClosedLocator(*this->value()); }
    size_t stack_index() const { return std::get<OpenLocator>(val).index; }
  };

  struct Closure {
    explicit Closure(function_ptr function) : function{function} {
      upvalues.reserve(function->upvalue_count);
    };
    function_ptr function;
    std::vector<upvalue_ptr> upvalues;
  };


  template<>
  void trace_references(const std::string* str, gc_heap* heap);

  template<>
  void trace_references(Function* func, gc_heap* heap);

  template<>
  void trace_references(NativeFn* func, gc_heap* heap);

  template<>
  void trace_references(RuntimeUpvalue* upvalue, gc_heap* heap);

  template<>
  void trace_references(Closure* closure, gc_heap* heap);
}