#include <string>

#include "cpplox/Bytecode/StringPool.h"

namespace cpplox {
const_string_ptr StringPool::insert_or_get(std::string_view sv) {
  auto iter = ptr_by_content.find(sv);
  if (iter != ptr_by_content.end()) {
    return iter->second;
  }
  const_string_ptr ptr {heap->make<const std::string>(sv.begin(), sv.end())};
  ptr_by_content[*ptr] = ptr;
  return ptr;
};

}; // namespace cpplox