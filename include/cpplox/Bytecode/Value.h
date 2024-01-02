#pragma once
#include <variant>

#include "GC.h"
#include "LoxObjectFwd.h"

namespace cpplox {

class Chunk;

using const_string_ptr    = gc_ptr<const std::string>;
using function_ptr        = gc_ptr<Function>;
using native_function_ptr = gc_ptr<NativeFn>;
using closure_ptr         = gc_ptr<Closure>;
using Value = std::variant<
  double, bool, std::monostate, const_string_ptr, function_ptr, native_function_ptr,
  closure_ptr>;
// Lightweight - can be passed around by value.

std::string to_string(const Value val);

struct GCValueMarkingVisitor {
  // Needed for the purpose of registering GC callbacks.
  gc_heap* heap;
  explicit GCValueMarkingVisitor(gc_heap* heap) : heap{heap} {};

  template<typename T>
  void operator()(const gc_ptr<T> ptr) const {
  #ifdef DEBUG_LOG_GC
    std::cout << "Marking gc_ptr<" << demangled_type_name<T>() << ">" << std::endl;
  #endif

    assert(ptr.get() != nullptr);
    heap->mark(ptr);
  }

  template<typename T>
  void operator()(const T value) const {
  #ifdef DEBUG_LOG_GC
    std::cout << "Skipping mark for: " << demangled_type_name<T>() << std::endl;
  #endif
  }
};

}  // namespace cpplox