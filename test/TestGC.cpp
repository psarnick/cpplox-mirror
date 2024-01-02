#include <unordered_map>
#include <type_traits>
#include "memory.h"

#include "cpplox/Bytecode/GC.h"
#include "cpplox//Bytecode/Value.h"
#include "cpplox/Bytecode/LoxObject.h"
#include "gtest/gtest.h"

namespace cpplox_tests {

using namespace cpplox;

TEST(GCTests, GcHeapCopyAndMoveSemantics) {
    static_assert(!std::is_copy_constructible<gc_heap>::value,
                  "gc_heap should not be copy constructible");

    static_assert(!std::is_copy_assignable<gc_heap>::value,
                  "gc_heap should not be copy assignable");

    static_assert(!std::is_move_constructible<gc_heap>::value,
                  "gc_heap should not be move constructible");

    static_assert(!std::is_move_assignable<gc_heap>::value,
                  "gc_heap should not be move assignable");
};

TEST(GCTests, GcPtrCopyAndMoveSemantics) {
    static_assert(std::is_trivially_copy_constructible<gc_ptr<Value>>::value,
                  "gc_ptr should be copy constructible");

    static_assert(std::is_trivially_copy_assignable<gc_ptr<Value>>::value,
                  "gc_ptr should be copy assignable");

    static_assert(std::is_trivially_move_constructible<gc_ptr<Value>>::value,
                  "gc_ptr should be move constructible");

    static_assert(std::is_trivially_move_assignable<gc_ptr<Value>>::value,
                  "gc_ptr should be move assignable");
};

TEST(GCTests, String) {
    gc_heap heap {};
    const_string_ptr ptr {heap.make<const std::string>("foobar")};
    
    EXPECT_EQ(*ptr, "foobar");
    EXPECT_EQ(heap.size(), 1);
};

TEST(GCTests, GCPtrUsage) {
    gc_heap heap {};
    const_string_ptr ptr1 {heap.make<const std::string>("foobar")};

    const_string_ptr copy_ptr1 {ptr1};
    EXPECT_FALSE(&ptr1 == &copy_ptr1);
    EXPECT_TRUE(ptr1 == copy_ptr1);
    EXPECT_TRUE(*ptr1 == *copy_ptr1);

    const_string_ptr copy_ptr2 = ptr1;
    EXPECT_FALSE(&ptr1 == &copy_ptr2);
    EXPECT_TRUE(ptr1 == copy_ptr2);
    EXPECT_TRUE(*ptr1 == *copy_ptr2);

    const_string_ptr copy_ptr3 = copy_ptr2;
    EXPECT_FALSE(&ptr1 == &copy_ptr3);
    EXPECT_TRUE(ptr1 == copy_ptr3);
    EXPECT_TRUE(*ptr1 == *copy_ptr3);

    std::unordered_map<const_string_ptr, bool> map;
    map.insert_or_assign(ptr1, true);
    EXPECT_TRUE(map.contains(ptr1));
    EXPECT_TRUE(map.contains(copy_ptr1));
    EXPECT_TRUE(map.contains(copy_ptr2));
    EXPECT_TRUE(map.contains(copy_ptr3));
};

TEST(GCTests, GCHeapUsage) {
    gc_heap heap {};

    const_string_ptr ptr1 {heap.make<const std::string>("foobar")};
    const_string_ptr ptr2 {heap.make<const std::string>("foobar")};
    EXPECT_TRUE(ptr1 != ptr2);
    // gc_heap does not deduplicate objects.

    std::unordered_map<const_string_ptr, Value> map;
    map.insert_or_assign(ptr1, 10.0);
    EXPECT_TRUE(std::get<double>(map.find(ptr1)->second) == 10.0);
    map.insert_or_assign(ptr2, false);
    EXPECT_TRUE(std::get<bool>(map.find(ptr2)->second) == false);
    map.insert_or_assign(ptr2, ptr1);
    EXPECT_TRUE(*std::get<const_string_ptr>(map.find(ptr2)->second) == "foobar");

    std::string const_fn_name {"test_fn"};
    const_string_ptr ptr3 {heap.make<const std::string>(const_fn_name)};
    function_ptr ptr4 {heap.make<Function>(0, 0, ptr3, std::make_unique<Chunk>())};
    map.insert_or_assign(ptr3, ptr4);
    EXPECT_TRUE(*std::get<function_ptr>(map.find(ptr3)->second)->name == const_fn_name);
};

};