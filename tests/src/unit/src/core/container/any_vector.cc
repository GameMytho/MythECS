#include <string>
#include <gtest/gtest.h>
#include <core/container/any_vector.hpp>

using namespace myth::core::container;

struct TestType {
    int x;
    float y;
    std::string str;
};

// ============================================================================
// Functionalities - empty, emplace_back, access, clear
// ============================================================================
TEST(AnyVector, Functionalities) {
    any_vector vec(myth::core::type_info_generator::gen<TestType>());

    // A fresh vector is empty with zero capacity.
    ASSERT_TRUE(vec.empty());
    ASSERT_EQ(vec.size(), 0);
    ASSERT_EQ(vec.capacity(), 0);

    TestType t1 { 1, 1.0f, "one" };

    vec.emplace_back(&t1);

    // After one emplace_back, size=1 and capacity is allocated.
    ASSERT_FALSE(vec.empty());
    ASSERT_EQ(vec.size(), 1);
    ASSERT_GE(vec.capacity(), 1);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->x, 1);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->y, 1.0f);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->str, "one");

    // Clear resets size but preserves allocated capacity.
    vec.clear();

    ASSERT_TRUE(vec.empty());
    ASSERT_EQ(vec.size(), 0);
    ASSERT_GE(vec.capacity(), 1);
}

// ============================================================================
// Constructor - type-erased storage via type_info
// ============================================================================
TEST(AnyVector, Constructor) {
    any_vector vec(myth::core::type_info_generator::gen<TestType>());

    ASSERT_TRUE(vec.empty());
    ASSERT_EQ(vec.size(), 0);
    ASSERT_EQ(vec.capacity(), 0);

    TestType t { 1, 1.0f, "one" };
    vec.emplace_back(&t);

    ASSERT_FALSE(vec.empty());
    ASSERT_EQ(vec.size(), 1);
    ASSERT_GE(vec.capacity(), 1);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->x, 1);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->y, 1.0f);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->str, "one");
}

// ============================================================================
// Move - resources transfer to destination; source is left empty
// ============================================================================
TEST(AnyVector, Move) {
    any_vector vec(myth::core::type_info_generator::gen<TestType>());

    TestType t { 1, 1.0f, "one" };
    vec.emplace_back(&t);

    // Move ctor - source becomes empty.
    any_vector vec2(std::move(vec));

    ASSERT_TRUE(vec.empty());

    ASSERT_FALSE(vec2.empty());
    ASSERT_EQ(vec2.size(), 1);
    ASSERT_GE(vec2.capacity(), 1);
    ASSERT_EQ(static_cast<TestType*>(vec2[0])->x, 1);
    ASSERT_EQ(static_cast<TestType*>(vec2[0])->y, 1.0f);
    ASSERT_EQ(static_cast<TestType*>(vec2[0])->str, "one");
}

// ============================================================================
// Emplace - multiple emplace_back calls grow the vector
// ============================================================================
TEST(AnyVector, Emplace) {
    any_vector vec(myth::core::type_info_generator::gen<TestType>());

    TestType t1 { 1, 1.0f, "one" };
    TestType t2 { 2, 2.0f, "two" };

    vec.emplace_back(&t1);
    vec.emplace_back(&t2);

    ASSERT_EQ(vec.size(), 2);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->x, 1);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->y, 1.0f);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->str, "one");
    ASSERT_EQ(static_cast<TestType*>(vec[1])->x, 2);
    ASSERT_EQ(static_cast<TestType*>(vec[1])->y, 2.0f);
    ASSERT_EQ(static_cast<TestType*>(vec[1])->str, "two");

    // Emplace a third element (duplicate of t1 is fine).
    vec.emplace_back(&t1);
    ASSERT_EQ(vec.size(), 3);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->x, 1);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->y, 1.0f);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->str, "one");
    ASSERT_EQ(static_cast<TestType*>(vec[1])->x, 2);
    ASSERT_EQ(static_cast<TestType*>(vec[1])->y, 2.0f);
    ASSERT_EQ(static_cast<TestType*>(vec[1])->str, "two");
    ASSERT_EQ(static_cast<TestType*>(vec[2])->x, 1);
    ASSERT_EQ(static_cast<TestType*>(vec[2])->y, 1.0f);
    ASSERT_EQ(static_cast<TestType*>(vec[2])->str, "one");

    // Emplace a fourth element.
    TestType t3 { 3, 3.0f, "three" };
    vec.emplace_back(&t3);

    ASSERT_EQ(vec.size(), 4);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->x, 1);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->y, 1.0f);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->str, "one");
    ASSERT_EQ(static_cast<TestType*>(vec[1])->x, 2);
    ASSERT_EQ(static_cast<TestType*>(vec[1])->y, 2.0f);
    ASSERT_EQ(static_cast<TestType*>(vec[1])->str, "two");
    ASSERT_EQ(static_cast<TestType*>(vec[2])->x, 1);
    ASSERT_EQ(static_cast<TestType*>(vec[2])->y, 1.0f);
    ASSERT_EQ(static_cast<TestType*>(vec[2])->str, "one");
    ASSERT_EQ(static_cast<TestType*>(vec[3])->x, 3);
    ASSERT_EQ(static_cast<TestType*>(vec[3])->y, 3.0f);
    ASSERT_EQ(static_cast<TestType*>(vec[3])->str, "three");
}

// ============================================================================
// Pop - pop_back removes elements in LIFO order
// ============================================================================
TEST(AnyVector, Pop) {
    any_vector vec(myth::core::type_info_generator::gen<TestType>());

    TestType t1 { 1, 1.0f, "one" };
    TestType t2 { 2, 2.0f, "two" };
    TestType t3 { 3, 3.0f, "three" };

    vec.emplace_back(&t1);
    vec.emplace_back(&t2);
    vec.emplace_back(&t3);

    ASSERT_FALSE(vec.empty());
    ASSERT_EQ(vec.size(), 3);

    // Pop from back - remaining elements unchanged.
    vec.pop_back();

    ASSERT_FALSE(vec.empty());
    ASSERT_EQ(vec.size(), 2);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->x, 1);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->y, 1.0f);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->str, "one");
    ASSERT_EQ(static_cast<TestType*>(vec[1])->x, 2);
    ASSERT_EQ(static_cast<TestType*>(vec[1])->y, 2.0f);
    ASSERT_EQ(static_cast<TestType*>(vec[1])->str, "two");

    vec.pop_back();

    ASSERT_FALSE(vec.empty());
    ASSERT_EQ(vec.size(), 1);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->x, 1);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->y, 1.0f);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->str, "one");

    // Pop last element - vector becomes empty.
    vec.pop_back();

    ASSERT_TRUE(vec.empty());
    ASSERT_EQ(vec.size(), 0);
}

// ============================================================================
// Swap - swap two elements by index via byte-level swap (type-erased)
// ============================================================================
TEST(AnyVector, Swap) {
    any_vector vec(myth::core::type_info_generator::gen<TestType>());

    TestType t1 { 1, 1.0f, "one" };
    TestType t2 { 2, 2.0f, "two" };
    TestType t3 { 3, 3.0f, "three" };

    vec.emplace_back(&t1);
    vec.emplace_back(&t2);
    vec.emplace_back(&t3);

    ASSERT_EQ(vec.size(), 3);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->x, 1);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->y, 1.0f);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->str, "one");
    ASSERT_EQ(static_cast<TestType*>(vec[1])->x, 2);
    ASSERT_EQ(static_cast<TestType*>(vec[1])->y, 2.0f);
    ASSERT_EQ(static_cast<TestType*>(vec[1])->str, "two");
    ASSERT_EQ(static_cast<TestType*>(vec[2])->x, 3);
    ASSERT_EQ(static_cast<TestType*>(vec[2])->y, 3.0f);
    ASSERT_EQ(static_cast<TestType*>(vec[2])->str, "three");

    // Swap indices 0 and 1.
    vec.swap(0, 1);

    ASSERT_EQ(vec.size(), 3);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->x, 2);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->y, 2.0f);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->str, "two");
    ASSERT_EQ(static_cast<TestType*>(vec[1])->x, 1);
    ASSERT_EQ(static_cast<TestType*>(vec[1])->y, 1.0f);
    ASSERT_EQ(static_cast<TestType*>(vec[1])->str, "one");
    ASSERT_EQ(static_cast<TestType*>(vec[2])->x, 3);
    ASSERT_EQ(static_cast<TestType*>(vec[2])->y, 3.0f);
    ASSERT_EQ(static_cast<TestType*>(vec[2])->str, "three");

    // Swap indices 1 and 2.
    vec.swap(1, 2);

    ASSERT_EQ(vec.size(), 3);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->x, 2);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->y, 2.0f);
    ASSERT_EQ(static_cast<TestType*>(vec[0])->str, "two");
    ASSERT_EQ(static_cast<TestType*>(vec[1])->x, 3);
    ASSERT_EQ(static_cast<TestType*>(vec[1])->y, 3.0f);
    ASSERT_EQ(static_cast<TestType*>(vec[1])->str, "three");
    ASSERT_EQ(static_cast<TestType*>(vec[2])->x, 1);
    ASSERT_EQ(static_cast<TestType*>(vec[2])->y, 1.0f);
    ASSERT_EQ(static_cast<TestType*>(vec[2])->str, "one");
}
