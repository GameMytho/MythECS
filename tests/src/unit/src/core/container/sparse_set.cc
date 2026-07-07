#include <utility>

#include <gtest/gtest.h>
#include <core/container/sparse_set.hpp>

using namespace myth::core::container;

// ============================================================================
// Functionalities - empty, emplace, contains, index, safe_index, operator[], clear
// ============================================================================
TEST(SparseSet, Functionalities) {
    sparse_set<uint32_t> set;

    // A default-constructed sparse_set is empty with zero capacity.
    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0);
    ASSERT_EQ(set.capacity(), 0);

    // Missing value: contains() is false, safe_index() returns sentinel.
    ASSERT_FALSE(set.contains(13));
    ASSERT_EQ(set.safe_index(13), sparse_set<uint32_t>::null_value_index);
    set.emplace_back(13);

    // After emplace, non-empty with allocated capacity.
    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 1);
    ASSERT_NE(set.capacity(), 0);

    // The emplaced value is reachable via contains / index / safe_index / operator[].
    ASSERT_TRUE(set.contains(13));
    ASSERT_EQ(set.index(13), 0);
    ASSERT_EQ(set.safe_index(13), 0);
    ASSERT_EQ(set[0], 13);

    // Clear resets size to zero but preserves allocated capacity.
    set.clear();

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0);
    ASSERT_NE(set.capacity(), 0);
    ASSERT_FALSE(set.contains(13));
    ASSERT_EQ(set.safe_index(13), sparse_set<uint32_t>::null_value_index);
}

// ============================================================================
// Constructors - default, capacity, copy, move
// ============================================================================
TEST(SparseSet, Constructors) {
    sparse_set<uint32_t> set1;

    // Assign a capacity-initialized set.
    set1 = sparse_set<uint32_t>{ 10 };

    ASSERT_EQ(set1.size(), 0);
    ASSERT_GE(set1.capacity(), 10);

    ASSERT_FALSE(set1.contains(13));
    ASSERT_EQ(set1.safe_index(13), sparse_set<uint32_t>::null_value_index);
    set1.emplace_back(13);

    // Copy-then-move construction chain.
    sparse_set<uint32_t> tmp { set1 };
    sparse_set<uint32_t> set2 { std::move(tmp) };

    ASSERT_EQ(set1.size(), 1);
    ASSERT_TRUE(set1.contains(13));
    ASSERT_EQ(set1.index(13), 0);
    ASSERT_EQ(set1.safe_index(13), 0);
    ASSERT_EQ(set1[0], 13);

    ASSERT_EQ(set2.size(), 1);
    ASSERT_TRUE(set2.contains(13));
    ASSERT_EQ(set2.index(13), 0);
    ASSERT_EQ(set2.safe_index(13), 0);
    ASSERT_EQ(set2[0], 13);
}

// ============================================================================
// Copy - copy ctor and copy assignment; sets are independent
// ============================================================================
TEST(SparseSet, Copy) {
    sparse_set<uint32_t> set1;

    ASSERT_FALSE(set1.contains(13));
    ASSERT_EQ(set1.safe_index(13), sparse_set<uint32_t>::null_value_index);
    set1.emplace_back(13);

    // Copy ctor - set2 is an independent copy.
    sparse_set<uint32_t> set2 { set1 };

    ASSERT_EQ(set1.size(), 1);
    ASSERT_TRUE(set1.contains(13));
    ASSERT_EQ(set1.index(13), 0);
    ASSERT_EQ(set1.safe_index(13), 0);
    ASSERT_EQ(set1[0], 13);

    ASSERT_EQ(set2.size(), 1);
    ASSERT_TRUE(set2.contains(13));
    ASSERT_EQ(set2.index(13), 0);
    ASSERT_EQ(set2.safe_index(13), 0);
    ASSERT_EQ(set2[0], 13);

    // Mutate independently - adding to one does not affect the other.
    ASSERT_FALSE(set1.contains(42));
    ASSERT_EQ(set1.safe_index(42), sparse_set<uint32_t>::null_value_index);
    set1.emplace_back(42);

    ASSERT_FALSE(set1.contains(100));
    ASSERT_EQ(set1.safe_index(100), sparse_set<uint32_t>::null_value_index);
    set1.emplace_back(100);

    ASSERT_FALSE(set2.contains(0));
    ASSERT_EQ(set1.safe_index(0), sparse_set<uint32_t>::null_value_index);
    set2.emplace_back(0);

    // Copy assignment - set2 replaced by set1's contents.
    set2 = set1;

    // set1 has {13, 42, 100} in insertion order (indices 0, 1, 2).
    ASSERT_EQ(set1.size(), 3);
    ASSERT_EQ(set1.index(13), 0);
    ASSERT_EQ(set1.index(42), 1);
    ASSERT_EQ(set1.index(100), 2);
    ASSERT_EQ(set1.safe_index(13), 0);
    ASSERT_EQ(set1.safe_index(42), 1);
    ASSERT_EQ(set1.safe_index(100), 2);
    ASSERT_TRUE(set1.contains(13));
    ASSERT_TRUE(set1.contains(42));
    ASSERT_TRUE(set1.contains(100));

    // set2 is now a copy of set1.
    ASSERT_EQ(set2.size(), 3);
    ASSERT_EQ(set2.index(13), 0);
    ASSERT_EQ(set2.index(42), 1);
    ASSERT_EQ(set2.index(100), 2);
    ASSERT_EQ(set2.safe_index(13), 0);
    ASSERT_EQ(set2.safe_index(42), 1);
    ASSERT_EQ(set2.safe_index(100), 2);
    ASSERT_TRUE(set2.contains(13));
    ASSERT_TRUE(set2.contains(42));
    ASSERT_TRUE(set2.contains(100));
}

// ============================================================================
// Move - move ctor and move assignment; source is left empty
// ============================================================================
TEST(SparseSet, Move) {
    sparse_set<uint32_t> set1;

    ASSERT_FALSE(set1.contains(13));
    ASSERT_EQ(set1.safe_index(13), sparse_set<uint32_t>::null_value_index);
    set1.emplace_back(13);

    // Move ctor - set1 resources transfer to set2.
    sparse_set<uint32_t> set2 { std::move(set1) };

    ASSERT_EQ(set1.size(), 0);
    ASSERT_FALSE(set1.contains(13));
    ASSERT_EQ(set1.safe_index(13), sparse_set<uint32_t>::null_value_index);

    ASSERT_EQ(set2.size(), 1);
    ASSERT_TRUE(set2.contains(13));
    ASSERT_EQ(set2.index(13), 0);
    ASSERT_EQ(set2.safe_index(13), 0);
    ASSERT_EQ(set2[0], 13);

    // Moved-from set can be reused after populating again.
    ASSERT_FALSE(set1.contains(42));
    ASSERT_EQ(set1.safe_index(42), sparse_set<uint32_t>::null_value_index);
    set1.emplace_back(42);

    ASSERT_FALSE(set1.contains(100));
    ASSERT_EQ(set1.safe_index(100), sparse_set<uint32_t>::null_value_index);
    set1.emplace_back(100);

    ASSERT_FALSE(set2.contains(0));
    ASSERT_EQ(set2.safe_index(0), sparse_set<uint32_t>::null_value_index);
    set2.emplace_back(0);

    // Move assignment - set1 resources transfer to set2.
    set2 = std::move(set1);

    ASSERT_EQ(set1.size(), 0);

    ASSERT_EQ(set2.size(), 2);
    ASSERT_EQ(set2.index(42), 0);
    ASSERT_EQ(set2.index(100), 1);
    ASSERT_EQ(set2.safe_index(42), 0);
    ASSERT_EQ(set2.safe_index(100), 1);
    ASSERT_TRUE(set2.contains(42));
    ASSERT_TRUE(set2.contains(100));
    ASSERT_EQ(set2[0], 42);
    ASSERT_EQ(set2[1], 100);
}

// ============================================================================
// Emplace - insert values and verify returned index
// ============================================================================
TEST(SparseSet, Emplace) {
    sparse_set<uint32_t> set;

    ASSERT_FALSE(set.contains(13));
    ASSERT_EQ(set.safe_index(13), sparse_set<uint32_t>::null_value_index);
    auto index = set.emplace_back(13);

    ASSERT_EQ(set.size(), 1);
    ASSERT_TRUE(set.contains(13));
    ASSERT_EQ(index, set.index(13));
    ASSERT_EQ(index, set.safe_index(13));
    ASSERT_EQ(set[index], 13);

    ASSERT_FALSE(set.contains(42));
    ASSERT_EQ(set.safe_index(42), sparse_set<uint32_t>::null_value_index);
    index = set.emplace_back(42);

    ASSERT_EQ(set.size(), 2);
    ASSERT_TRUE(set.contains(42));
    ASSERT_EQ(index, set.index(42));
    ASSERT_EQ(index, set.safe_index(42));
    ASSERT_EQ(set[index], 42);

    ASSERT_FALSE(set.contains(100));
    ASSERT_EQ(set.safe_index(100), sparse_set<uint32_t>::null_value_index);
    index = set.emplace_back(100);

    ASSERT_TRUE(set.contains(100));
    ASSERT_EQ(set.size(), 3);
    ASSERT_EQ(index, set.index(100));
    ASSERT_EQ(index, set.safe_index(100));
    ASSERT_EQ(set[index], 100);

    ASSERT_FALSE(set.contains(0));
    ASSERT_EQ(set.safe_index(0), sparse_set<uint32_t>::null_value_index);
    index = set.emplace_back(0);

    ASSERT_EQ(set.size(), 4);
    ASSERT_TRUE(set.contains(0));
    ASSERT_EQ(index, set.index(0));
    ASSERT_EQ(index, set.safe_index(0));
    ASSERT_EQ(set[index], 0);
}

// ============================================================================
// Erase - remove values; back-fill compacts the dense array, sparse index updated
// ============================================================================
TEST(SparseSet, Erase) {
    sparse_set<uint32_t> set;

    ASSERT_FALSE(set.contains(13));
    ASSERT_EQ(set.safe_index(13), sparse_set<uint32_t>::null_value_index);
    set.emplace_back(13);

    ASSERT_FALSE(set.contains(42));
    ASSERT_EQ(set.safe_index(42), sparse_set<uint32_t>::null_value_index);
    set.emplace_back(42);

    ASSERT_FALSE(set.contains(100));
    ASSERT_EQ(set.safe_index(100), sparse_set<uint32_t>::null_value_index);
    set.emplace_back(100);

    ASSERT_FALSE(set.contains(0));
    ASSERT_EQ(set.safe_index(0), sparse_set<uint32_t>::null_value_index);
    set.emplace_back(0);

    ASSERT_EQ(set.size(), 4);
    ASSERT_EQ(set.index(13), 0);
    ASSERT_EQ(set.index(42), 1);
    ASSERT_EQ(set.index(100), 2);
    ASSERT_EQ(set.index(0), 3);
    ASSERT_EQ(set.safe_index(13), 0);
    ASSERT_EQ(set.safe_index(42), 1);
    ASSERT_EQ(set.safe_index(100), 2);
    ASSERT_EQ(set.safe_index(0), 3);

    // Erase 42 (index 1) - back-filled by 0 (was at index 3).
    set.erase(42);

    ASSERT_EQ(set.size(), 3);
    ASSERT_EQ(set.safe_index(42), sparse_set<uint32_t>::null_value_index);
    ASSERT_EQ(set.index(13), 0);
    ASSERT_EQ(set.index(0), 1);    // 0 moved up
    ASSERT_EQ(set.index(100), 2);

    // Erase 13 (index 0) - back-filled by 100 (was at index 2).
    set.erase(13);

    ASSERT_EQ(set.size(), 2);
    ASSERT_EQ(set.safe_index(42), sparse_set<uint32_t>::null_value_index);
    ASSERT_EQ(set.safe_index(13), sparse_set<uint32_t>::null_value_index);
    ASSERT_EQ(set.index(100), 0);  // 100 moved up
    ASSERT_EQ(set.index(0), 1);

    // Erase 0 (index 1) - now it is the back element, direct pop.
    set.erase(0);

    ASSERT_EQ(set.size(), 1);
    ASSERT_EQ(set.safe_index(42), sparse_set<uint32_t>::null_value_index);
    ASSERT_EQ(set.safe_index(13), sparse_set<uint32_t>::null_value_index);
    ASSERT_EQ(set.safe_index(0), sparse_set<uint32_t>::null_value_index);
    ASSERT_EQ(set.index(100), 0);

    // Erase 100 - last element, set becomes empty.
    set.erase(100);

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0);
    ASSERT_EQ(set.safe_index(42), sparse_set<uint32_t>::null_value_index);
    ASSERT_EQ(set.safe_index(13), sparse_set<uint32_t>::null_value_index);
    ASSERT_EQ(set.safe_index(100), sparse_set<uint32_t>::null_value_index);
    ASSERT_EQ(set.safe_index(0), sparse_set<uint32_t>::null_value_index);
}

// ============================================================================
// Swap - swap two elements by dense index; sparse index table updated
// ============================================================================
TEST(SparseSet, Swap) {
    sparse_set<uint32_t> set;

    ASSERT_EQ(set.safe_index(13), sparse_set<uint32_t>::null_value_index);
    ASSERT_EQ(set.safe_index(42), sparse_set<uint32_t>::null_value_index);
    ASSERT_EQ(set.safe_index(100), sparse_set<uint32_t>::null_value_index);

    set.emplace_back(13);
    set.emplace_back(42);
    set.emplace_back(100);

    ASSERT_EQ(set.size(), 3);
    ASSERT_EQ(set[0], 13);
    ASSERT_EQ(set[1], 42);
    ASSERT_EQ(set[2], 100);
    ASSERT_EQ(set.index(13), 0);
    ASSERT_EQ(set.index(42), 1);
    ASSERT_EQ(set.index(100), 2);
    ASSERT_EQ(set.safe_index(13), 0);
    ASSERT_EQ(set.safe_index(42), 1);
    ASSERT_EQ(set.safe_index(100), 2);

    // Swap indices 0 and 2: values exchange places, sparse index updated.
    set.swap(0, 2);

    ASSERT_EQ(set.size(), 3);
    ASSERT_EQ(set[0], 100);
    ASSERT_EQ(set[1], 42);
    ASSERT_EQ(set[2], 13);
    ASSERT_EQ(set.index(100), 0);
    ASSERT_EQ(set.index(42), 1);
    ASSERT_EQ(set.index(13), 2);
    ASSERT_EQ(set.safe_index(100), 0);
    ASSERT_EQ(set.safe_index(42), 1);
    ASSERT_EQ(set.safe_index(13), 2);

    // Swap indices 0 and 1.
    set.swap(0, 1);

    ASSERT_EQ(set[0], 42);
    ASSERT_EQ(set[1], 100);
    ASSERT_EQ(set.index(42), 0);
    ASSERT_EQ(set.index(100), 1);
    ASSERT_EQ(set.safe_index(42), 0);
    ASSERT_EQ(set.safe_index(100), 1);

    // Self-swap - must be a no-op.
    set.swap(1, 1);

    ASSERT_EQ(set[0], 42);
    ASSERT_EQ(set[1], 100);
    ASSERT_EQ(set.index(42), 0);
    ASSERT_EQ(set.index(100), 1);
    ASSERT_EQ(set.safe_index(42), 0);
    ASSERT_EQ(set.safe_index(100), 1);
}
