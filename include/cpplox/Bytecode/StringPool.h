#pragma once
#include "GC.h"
#include "cpplox/Bytecode/Value.h"
#include "cpplox/Bytecode/LoxObject.h"

#include <string_view>
#include <unordered_map>

namespace cpplox {
  class StringPool {
    // TODO: pool needs to be aware of garbage collection, otherwise ptr_by_content
    // will return dangling pointer if the const_string_ptr object was destructed.
    // Options: - add weak_refs to gc_heap & have string StringPool check if object
    //            is alive before returning it.
    //          - allow custom destructors for gc_cells? Note: gc_ptr is a non-owning
    //            pointer, so custom destructor there won't help.
    //          - make gc_heap accept callbacks invoked before objects are destroyed.
    gc_heap* const heap;   
    std::unordered_map<std::string_view, const_string_ptr> ptr_by_content {};
      
  public:
    StringPool()                             = delete;
    StringPool(StringPool&)                  = delete;
    StringPool& operator=(const StringPool&) = delete;
    StringPool(StringPool&&)                 = delete;
    StringPool& operator=(StringPool&&)      = delete;
    ~StringPool()                            = default;
    // Default copy and move assignments are not generated due to gc_heap* const member.
    // Deleting them to be explicit. Copy construction could lead to weird bugs as
    // both StringPools would share gc_heap, but have separate caches. Deleting copy
    // ops disables move ops too (Hinnant: https://stackoverflow.com/questions/37092864)
    // but deleting to be explicit.

    explicit StringPool(gc_heap* const heap) : heap{heap} {};
    const_string_ptr insert_or_get(std::string_view sv);
  };

}; // namespace cpplox