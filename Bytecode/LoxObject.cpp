#include <iostream>

#include "cpplox/Bytecode/common.h"
#include "cpplox/Bytecode/LoxObject.h"


namespace cpplox {
  Value* RuntimeUpvalue::value() {
    if (auto* loc = std::get_if<OpenLocator>(&val)) {
      assert(loc->index < loc->stack.size());
      return &loc->stack[loc->index];
    } else {
      return &std::get<ClosedLocator>(val);
    } 
  }

  // TODO: Test those.
  template<>
  void trace_references(const std::string* str, gc_heap* heap) {
  #ifdef DEBUG_LOG_GC
    std::cout << "[trace_references]: const std::string* " << *str << " NOOP" << std::endl;
  #endif
  }

  template<>
  void trace_references(Function* func, gc_heap* heap) {
  #ifdef DEBUG_LOG_GC
    std::cout << "[trace_references]: <fn " << *func->name << ">" << std::endl;
  #endif

    heap->mark(func->name);
    const auto vmv {GCValueMarkingVisitor(heap)};
    for (const auto val : func->chunk->constants) {
      std::visit(vmv, val);
    }
  }

  template<>
  void trace_references(NativeFn* func, gc_heap* heap) {
  #ifdef DEBUG_LOG_GC
    std::cout << "[trace_references]: <native fn> NOOP" << std::endl;
  #endif
  }

  template<>
  void trace_references(RuntimeUpvalue* upvalue, gc_heap* heap) {
    // TODO: Take this as const RuntimeUpvalue once read-only access is added.
  #ifdef DEBUG_LOG_GC
    std::cout << "[trace_references]: RuntimeUpvalue " << to_string(*upvalue->value()) << std::endl;
  #endif
    
  }

  template<>
  void trace_references(Closure* closure, gc_heap* heap) {
  #ifdef DEBUG_LOG_GC
    std::cout << "[trace_references]: Closure for <fn " << *closure->function->name << ">" << std::endl;
  #endif

    trace_references(closure->function.get(), heap);
    const auto vmv {GCValueMarkingVisitor(heap)};
    for (const auto uv : closure->upvalues) {
      std::visit(vmv, *uv->value());
    }
  }
} //namespace cpplox