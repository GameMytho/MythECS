#include <utility>

#include <gtest/gtest.h>
#include <core/container/sparse_set.hpp>

using namespace myth::core::container;

TEST(SparseSet, Functionalities) {
    sparse_set<uint32_t> set;

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0);
    ASSERT_EQ(set.capacity(), 0);

    ASSERT_FALSE(set.contains(13));
    set.emplace(13);

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 1);
    ASSERT_NE(set.capacity(), 0);

    ASSERT_TRUE(set.contains(13));
    ASSERT_EQ(set.index(13), 0);
    ASSERT_EQ(set[0], 13);

    set.clear();

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0);
    ASSERT_NE(set.capacity(), 0);
    ASSERT_FALSE(set.contains(13));
}

TEST(SparseSet, Constructors) {
    sparse_set<uint32_t> set1;

    set1 = sparse_set<uint32_t>{ 10 };

    ASSERT_EQ(set1.size(), 0);
    ASSERT_GE(set1.capacity(), 10);

    ASSERT_FALSE(set1.contains(13));
    set1.emplace(13);

    sparse_set<uint32_t> tmp { set1 };
    sparse_set<uint32_t> set2 { std::move(tmp) };

    ASSERT_EQ(set1.size(), 1);
    ASSERT_TRUE(set1.contains(13));
    ASSERT_EQ(set1[0], 13);
    ASSERT_EQ(set2.size(), 1);
    ASSERT_TRUE(set2.contains(13));
    ASSERT_EQ(set2[0], 13);
}

TEST(SparseSet, Copy) {
    sparse_set<uint32_t> set1;

    ASSERT_FALSE(set1.contains(13));
    set1.emplace(13);

    sparse_set<uint32_t> set2 { set1 };

    ASSERT_EQ(set1.size(), 1);
    ASSERT_TRUE(set1.contains(13));
    ASSERT_EQ(set2.size(), 1);
    ASSERT_TRUE(set2.contains(13));

    ASSERT_FALSE(set1.contains(42));
    set1.emplace(42);
    ASSERT_FALSE(set1.contains(100));
    set1.emplace(100);
    ASSERT_FALSE(set2.contains(0));
    set2.emplace(0);
    set2 = set1;

    ASSERT_EQ(set1.size(), 3);
    ASSERT_TRUE(set1.contains(13));
    ASSERT_TRUE(set1.contains(42));
    ASSERT_TRUE(set1.contains(100));

    ASSERT_EQ(set2.size(), 3);
    ASSERT_TRUE(set2.contains(13));
    ASSERT_TRUE(set2.contains(42));
    ASSERT_TRUE(set2.contains(100));
}

TEST(SparseSet, Move) {
    sparse_set<uint32_t> set1;

    ASSERT_FALSE(set1.contains(13));
    set1.emplace(13);

    sparse_set<uint32_t> set2 { std::move(set1) };

    ASSERT_EQ(set1.size(), 0);
    ASSERT_FALSE(set1.contains(13));
    ASSERT_EQ(set2.size(), 1);
    ASSERT_TRUE(set2.contains(13));

    ASSERT_FALSE(set1.contains(42));
    set1.emplace(42);
    ASSERT_FALSE(set1.contains(100));
    set1.emplace(100);
    ASSERT_FALSE(set2.contains(0));
    set2.emplace(0);
    set2 = std::move(set1);

    ASSERT_EQ(set1.size(), 0);
    ASSERT_EQ(set2.size(), 2);
    ASSERT_TRUE(set2.contains(42));
    ASSERT_TRUE(set2.contains(100));
    ASSERT_EQ(set2[0], 42);
    ASSERT_EQ(set2[1], 100);
}

TEST(SparseSet, Emplace) {
    sparse_set<uint32_t> set;

    ASSERT_FALSE(set.contains(13));
    auto index = set.emplace(13);

    ASSERT_EQ(set.size(), 1);
    ASSERT_TRUE(set.contains(13));
    ASSERT_EQ(index, set.index(13));
    ASSERT_EQ(set[index], 13);

    ASSERT_FALSE(set.contains(42));
    index = set.emplace(42);

    ASSERT_EQ(set.size(), 2);
    ASSERT_TRUE(set.contains(42));
    ASSERT_EQ(index, set.index(42));
    ASSERT_EQ(set[index], 42);

    ASSERT_FALSE(set.contains(100));
    index = set.emplace(100);

    ASSERT_TRUE(set.contains(100));
    ASSERT_EQ(set.size(), 3);
    ASSERT_EQ(index, set.index(100));
    ASSERT_EQ(set[index], 100);

    ASSERT_FALSE(set.contains(0));
    index = set.emplace(0);

    ASSERT_EQ(set.size(), 4);
    ASSERT_TRUE(set.contains(0));
    ASSERT_EQ(index, set.index(0));
    ASSERT_EQ(set[index], 0);
}

TEST(SparseSet, Erase) {
    sparse_set<uint32_t> set;

    ASSERT_FALSE(set.contains(13));
    set.emplace(13);
    ASSERT_FALSE(set.contains(42));
    set.emplace(42);
    ASSERT_FALSE(set.contains(100));
    set.emplace(100);
    ASSERT_FALSE(set.contains(0));
    set.emplace(0);

    ASSERT_EQ(set.size(), 4);
    ASSERT_EQ(set.index(13), 0);
    ASSERT_EQ(set.index(42), 1);
    ASSERT_EQ(set.index(100), 2);
    ASSERT_EQ(set.index(0), 3);

    set.erase(42);

    ASSERT_EQ(set.size(), 3);
    ASSERT_EQ(set.index(13), 0);
    ASSERT_EQ(set.index(0), 1);
    ASSERT_EQ(set.index(100), 2);

    set.erase(13);

    ASSERT_EQ(set.size(), 2);
    ASSERT_EQ(set.index(100), 0);
    ASSERT_EQ(set.index(0), 1);

    set.erase(0);

    ASSERT_EQ(set.size(), 1);
    ASSERT_EQ(set.index(100), 0);

    set.erase(100);

    ASSERT_TRUE(set.empty());
}