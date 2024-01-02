#include <string>
#include <iostream>
#include <typeinfo>

#include "cpplox/Bytecode/GC.h"
#include "cpplox/Bytecode/common.h"
#include "cpplox/Bytecode/Value.h"

namespace cpplox {
  void gc_heap::collect() {
  #ifdef DEBUG_LOG_GC
    std::cout << "=== gc begin ===" << std::endl;
    std::cout << "Root marking callbacks: " << root_marking_callbacks.size() << std::endl;
  #endif
    // Invariant: upon 1st call to .collect() no gc_cell is yet marked (as per construction).
    
    for (auto& cb : root_marking_callbacks) {
      cb();
    }
    // Sets is_marked to true for root objects. Makes one step from each node.

    while (!reachable.empty()) {
      std::cout << "[before] reachable.size(): " << reachable.size() << std::endl;
      auto node = reachable.back();
      std::cout << "[processing]: cell addr: " << node->addr() << std::endl;
      reachable.pop_back();
      node->invoke_trace_references(this);
      std::cout << "[after] reachable.size(): " << reachable.size() << std::endl << std::endl;
    }
    // Reaches to anything still alive.
    std::vector<std::unique_ptr<gc_cell_base>> to_keep;
    for (auto& cell : cells) {
      if (cell->is_marked) {
        to_keep.emplace_back(std::move(cell));
        to_keep.back()->is_marked = false;
        // Resets the invariant.
      } else {
      #ifdef DEBUG_LOG_GC
        std::cout << cell->addr() << " staged for deletion: " << cell->type_name() << std::endl;
      #endif
      }
    }
    cells = std::move(to_keep);
    // TODO: Notify StringPool to avoid dangling. This probably causes the crashes now.

  #ifdef DEBUG_LOG_GC
    std::cout << "==/ gc end /==" << std::endl << std::endl;
  #endif

  };

  void gc_heap::mark_internal(gc_cell_base* cell) {
    if (cell == nullptr || cell->is_marked) { 
      return;
    }
    cell->is_marked = true;
    reachable.push_back(cell);
  }

  void gc_heap::debug_print() const {
    std::cout << std::endl << " === debug_print === " << std::endl;
    std::cout << "Num cells: " << cells.size() << std::endl;
    for (auto& cell : cells) {
      std::cout << cell << " " << cell->type_name() << std::endl;
    }
    std::cout << " ==/ debug_print /== " << std::endl << std::endl;
  }

  gc_heap::~gc_heap() {
  #ifdef DEBUG_LOG_GC
    std::cout << "Destroying gc_heap" << std::endl;
    std::cout << "Number of roots before destruction: " << cells.size() << std::endl;
    std::cout << "gc_heap destroyed" << std::endl;
  #endif
    // Both vectors will automatically clean up owned resources. No need to .clear().
  }
}; // namespace cpplox