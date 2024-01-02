#include <unordered_map>
#include <type_traits>

#include "cpplox/Bytecode/StringPool.h"
#include "gtest/gtest.h"

namespace cpplox_tests {

using namespace cpplox;

TEST(StringPoolTests, StringPoolCopyAndMoveSemantics) {
  static_assert(!std::is_copy_constructible<StringPool>::value,
                "StringPool should not be copy constructible");

  static_assert(!std::is_copy_assignable<StringPool>::value,
                "StringPool should not be copy assignable");

  static_assert(!std::is_move_constructible<StringPool>::value,
                "StringPool should not be move constructible");

  static_assert(!std::is_move_assignable<StringPool>::value,
                "StringPool should not be move assignable");
};

TEST(StringPoolTests, Interning) {
  gc_heap heap {};
  StringPool pool {&heap};

  const_string_ptr ptr1 {pool.insert_or_get("foobar")};
  const_string_ptr ptr2 {pool.insert_or_get("foobar")};
  
  EXPECT_EQ(ptr1, ptr2);
  EXPECT_EQ(*ptr1, "foobar");
};

};