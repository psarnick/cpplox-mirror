#pragma once
// TODO: Dependencies break when this pragma is removed - any circular ones?
// Draw dependency graph with come c++ tool.
#include <cassert>
#include <vector>
#include <memory>
#include <type_traits>
#include <variant>
#include <functional>

#include "common.h"
// TODO: Rename common to Common

#ifdef DEBUG_LOG_GC

// TODO: Remove, this is for debugging segfaults only
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
// TODO: Remove, this is for debugging segfaults only

#include <iostream>
#include <variant>
#include "cpplox/Bytecode/LoxObjectFwd.h"

#endif


namespace cpplox {
  

  // NOTE: this class + its VM interface are a construction zone and nothing here is sacred.


  // Following https://youtu.be/JfmTagWcqoE?si=r2IXvHlWRieyk12e&t=2649 by exposing gc_ptr, a dedicated
  // type instead of a raw T* to provide correctness by construction. For now I started with the
  // simplest type erasure implementation based on https://youtu.be/tbUCHifyT24?si=EthmqV9azUdOCH5l&t=1696,
  // but I don't like how much indirection is necessary in current implementation.
  // This is not a fully generic memory mgmt system, so I don't want to register/unregister pointers on every
  // operation like https://github.com/hsutter/gcpp does. 
  
  // TODO: Think about abstrations that are simpler to interact with.

  // Note: current implementation does not set handed out pointers to null when gc_heap is destroyed. This
  // is fine for the purpose of the VM, as gc_heap is alive throughout execution.
  // Note: Giving users a direct access to raw pointer address makes it impossible to move objects closer
  // together to improve cache locality down the line.

  using gc_root_marking_cb = std::function<void()>;
  // TOOD: Possible to do this without coupling lifetimes? If not make the contract regarding coupling lifetimes
  // cleaner: callback has to be deregistered if registering object is destroyed (otherwise dangling).
  
  class gc_heap;
  struct gc_cell_base {
      virtual ~gc_cell_base() = default;
      virtual void invoke_trace_references(gc_heap*) const = 0;

    #ifdef DEBUG_LOG_GC
      virtual std::string addr() const = 0;
      virtual const std::string type_name() const = 0;
      // TODO: Remove as just for debugging.
    #endif
      
      bool is_marked {false};
  };

  template<typename T>
  struct gc_cell : public gc_cell_base {
      std::unique_ptr<T> data;

      gc_cell(std::unique_ptr<T> data_in) : data(std::move(data_in)) {};
      
      T* get() const { return data.get(); }
      
      void invoke_trace_references(gc_heap* heap) const override {
        trace_references(get(), heap);
      }

    #ifdef DEBUG_LOG_GC
      std::string addr() const override { 
        std::ostringstream oss;
        oss << static_cast<const void*>(data.get());
        return oss.str();
      }
      const std::string type_name() const override {
        return demangled_type_name<T>();
      }
    #endif
  };
  // TODO: Right now gc_cell and gc_cell_base leak to consumers,
  // isolate this.

  template<typename T> class gc_ptr;
  class gc_heap {
    // TODO: Move func definitions to .cpp.
    // TODO: Maybe debug output to something else than cout

    template<typename T> friend class gc_ptr;

    std::vector<std::unique_ptr<gc_cell_base>> cells;
    // TODO: Can remove unique_ptr and store by Value?
  
    std::vector<gc_root_marking_cb> root_marking_callbacks;
    std::vector<gc_cell_base*> reachable{};

  public:
    gc_heap()                               = default;
    gc_heap(const gc_heap& other)           = delete;
    gc_heap operator=(const gc_heap& other) = delete;
    // Copying is not well defined: even if resources stored in unique_ptrs could be copied,
    // gc_ptrs that were already handed out would still point to objects in the original gc_heap.

    gc_heap(gc_heap&&)                      = delete;
    gc_heap& operator=(gc_heap&&)           = delete;
    // Move semantics disabled until necessary: although =delete on copy operations counts as
    // user-defined and prevents compiler from implicitly generating default move ops, better
    // to be explicit. https://howardhinnant.github.io/classdecl.html

    ~gc_heap();

    void register_root_marking_callback(gc_root_marking_cb cb) { root_marking_callbacks.push_back(cb); }
    void deregister_root_marking_callback() { root_marking_callbacks.pop_back(); }
    // TODO: Brittle implementation but ok for now: compilers have FIFO behaviour, so this will work &
    // there's only one VM. Move this to dctors if going with callbacks for root marking.
    
    template<typename T>
    void mark(gc_ptr<T> ptr)  {
    #ifdef DEBUG_LOG_GC
      std::cout << "gc_heap::mark for " + ptr.cell->type_name() << std::endl;
    #endif
      mark_internal(ptr.cell);
    }

    void mark_internal(gc_cell_base* cell);

    template<typename T, typename ...Args>
    gc_ptr<T> make(Args&&... args) {
    #ifdef DEBUG_STRESS_GC
      if (cells.size() > 4) {collect();}
    #endif

      auto cell = std::make_unique<gc_cell<T>>(
        std::make_unique<T>(std::forward<Args>(args)...)
      );
      gc_cell<T>* cell_ptr {cell.get()};
      cells.emplace_back(std::move(cell));

    #ifdef DEBUG_LOG_GC
      std::cout << cell_ptr->data.get() << " allocated " << sizeof(T) << " for "
                << demangled_type_name<T>() << std::endl;
      // TODO: Print the right address & ensure sizeof will give the appopriate size.
      debug_print();
    #endif
      return gc_ptr<T>(cell_ptr->data.get(), cell_ptr);
    }

    size_t size() const { return cells.size(); }
    void collect();
    void debug_print() const;
  };

 // Typed, non-owning pointer exposed to users.
  template<typename T>
  class gc_ptr {
    friend class gc_heap;

    T* data {nullptr};
    gc_cell<T>* cell {nullptr};
    // TODO: Wrap in a single type as those cannot point independently.
    explicit gc_ptr(T* data_in, gc_cell<T>* cell) : data{data_in}, cell{cell} {};

  public:
    gc_ptr()                               = default;
    // Default constructor, ptrs already initialized.

    gc_ptr(const gc_ptr& other)            = default;
    gc_ptr& operator=(const gc_ptr& other) = default;
    // Trivial construction and assignment from other gc_ptr as all lifecycle is managed by gc_heap.

    gc_ptr(gc_ptr&&)                       = default;
    gc_ptr& operator=(gc_ptr&&)            = default;
    // Explicitly stating compiler defaults. Note: gc_ptr is move-enabled even if T is not.
    // Reason: moving gc_ptr is just moving the pointer, not the underlying data.
    ~gc_ptr()                              = default;

    T* get() const noexcept { return data; }

    T& operator*() const noexcept {
      assert (get() && "Attempt to dereference null");
      return *get();
    }

    T* operator->() const noexcept {
      assert (get() && "Attempt to dereference null");
      return get();
    }
  };

  template<typename T>
  bool operator==(gc_ptr<T> lhs, gc_ptr<T> rhs) {
    return lhs.get() == rhs.get();
    // Compares like a raw pointer, see GCTests::GCPtrUsage.
  }
  
  template<typename T>
  void trace_references(T obj, gc_heap* heap) {
    // TODO: tracing_references on T instead of gc_ptr<T> is messy as objects of type T
    // do not have to be allocated via gc_heap (const_string_ptr being one example).
    // At minimum: declare the interface on one of gc_heap's classes (gc_ptr<T>, cell).

    static_assert(
      std::is_same<T, void>::value, "should provide its own trace_references specialization");
    // Prevent silent failures. Users need to specialize this function for types that need
    // custom marking, even if action is just noop.
  }
} // namespace cpplox

template<typename T>
struct std::hash<cpplox::gc_ptr<T>> {
  std::size_t operator()(const cpplox::gc_ptr<T> p) const {
    return std::hash<T*>{}(p.get());
  }
  // Hashes like a raw pointer, see GCTests::GCPtrUsage.
};

