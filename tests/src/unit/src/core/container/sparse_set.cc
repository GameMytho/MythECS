#include <utility>

#include <gtest/gtest.h>
#include <core/container/sparse_set.hpp>

using namespace myth::core::container;

TEST(SparseSet, Functionalities) {
    sparse_set<uint32_t> set;

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0);
    ASSERT_EQ(set.capacity(), 0);
    ASSERT_EQ(set.page_count(), 0);
    ASSERT_EQ(set.page_capacity(), 0);
    ASSERT_EQ(set.cbegin(), set.cend());
    ASSERT_EQ(set.crbegin(), set.crend());

    ASSERT_FALSE(set.contains(13));
    set.emplace(13);

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 1);
    ASSERT_NE(set.capacity(), 0);
    ASSERT_NE(set.page_count(), 0);
    ASSERT_NE(set.page_capacity(), 0);
    ASSERT_EQ(set.cbegin(), set.cend() - 1);
    ASSERT_EQ(set.crbegin(), set.crend() - 1);

    ASSERT_TRUE(set.contains(13));
    ASSERT_EQ(set.index(13), 0);
    ASSERT_EQ(set[0], 13);

    set.clear();

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0);
    ASSERT_NE(set.capacity(), 0);
    ASSERT_EQ(set.page_count(), 0);
    ASSERT_NE(set.page_capacity(), 0);
    ASSERT_EQ(set.cbegin(), set.cend());
    ASSERT_EQ(set.crbegin(), set.crend());
    ASSERT_FALSE(set.contains(13));
}

TEST(SparseSet, Constructors) {
    sparse_set<uint32_t> set1;

    set1 = sparse_set<uint32_t>{ 100, 10 };

    ASSERT_EQ(set1.size(), 0);
    ASSERT_GE(set1.capacity(), 100);
    ASSERT_GE(set1.page_capacity(), 10);

    ASSERT_FALSE(set1.contains(13));
    set1.emplace(13);

    sparse_set<uint32_t> tmp { set1 };
    sparse_set<uint32_t> set2 { std::move(tmp) };

    ASSERT_EQ(set1.size(), 1);
    ASSERT_TRUE(set1.contains(13));
    ASSERT_EQ(set2.size(), 1);
    ASSERT_TRUE(set2.contains(13));
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
    ASSERT_EQ(*set1.cbegin(), 13);
    ASSERT_EQ(*(set1.cbegin() + 1), 42);
    ASSERT_EQ(*(set1.cbegin() + 2), 100);

    ASSERT_EQ(set2.size(), 3);
    ASSERT_TRUE(set2.contains(13));
    ASSERT_TRUE(set2.contains(42));
    ASSERT_TRUE(set2.contains(100));
    ASSERT_EQ(*set2.cbegin(), 13);
    ASSERT_EQ(*(set2.cbegin() + 1), 42);
    ASSERT_EQ(*(set2.cbegin() + 2), 100);
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
    ASSERT_EQ(*set2.cbegin(), 42);
    ASSERT_EQ(*(set2.cbegin() + 1), 100);
}

TEST(SparseSet, ConstIterator) {
    using iterator_type = sparse_set<uint32_t>::const_iterator_type;

    sparse_set<uint32_t> set;

    ASSERT_FALSE(set.contains(13));
    set.emplace(13);

    iterator_type cend{set.cbegin()};
    iterator_type cbegin{};
    cbegin = set.cend();
    std::swap(cbegin, cend);

    ASSERT_EQ(cbegin, set.cbegin());
    ASSERT_EQ(cend, set.cend());
    ASSERT_NE(cbegin, cend);

    ASSERT_EQ(cbegin++, set.cbegin());
    ASSERT_EQ(cbegin--, set.cend());

    ASSERT_EQ(cbegin + 1, set.cend());
    ASSERT_EQ(cend - 1, set.cbegin());

    ASSERT_EQ(++cbegin, set.cend());
    ASSERT_EQ(--cbegin, set.cbegin());

    ASSERT_EQ(cbegin += 1, set.cend());
    ASSERT_EQ(cbegin -= 1, set.cbegin());

    ASSERT_EQ(cbegin + (cend - cbegin), set.cend());
    ASSERT_EQ(cbegin - (cbegin - cend), set.cend());

    ASSERT_EQ(cend - (cend - cbegin), set.cbegin());
    ASSERT_EQ(cend + (cbegin - cend), set.cbegin());

    ASSERT_EQ(cbegin[0u], *set.cbegin().operator->());
    ASSERT_EQ(cbegin[0u], *set.cbegin());

    ASSERT_LT(cbegin, cend);
    ASSERT_LE(cbegin, set.cbegin());

    ASSERT_GT(cend, cbegin);
    ASSERT_GE(cend, set.cend());

    ASSERT_FALSE(set.contains(42));
    set.emplace(42);
    cbegin = set.cbegin();

    ASSERT_EQ(cbegin[0u], 13);
    ASSERT_EQ(cbegin[1u], 42);
}

TEST(SparseSet, ConstReverseIterator) {
    using iterator_type = sparse_set<uint32_t>::const_reverse_iterator_type;

    sparse_set<uint32_t> set;

    ASSERT_FALSE(set.contains(13));
    set.emplace(13);

    iterator_type crend{set.crbegin()};
    iterator_type crbegin{};
    crbegin = set.crend();
    std::swap(crbegin, crend);

    ASSERT_EQ(crbegin, set.crbegin());
    ASSERT_EQ(crend, set.crend());
    ASSERT_NE(crbegin, crend);

    ASSERT_EQ(crbegin++, set.crbegin());
    ASSERT_EQ(crbegin--, set.crend());

    ASSERT_EQ(crbegin + 1, set.crend());
    ASSERT_EQ(crend - 1, set.crbegin());

    ASSERT_EQ(++crbegin, set.crend());
    ASSERT_EQ(--crbegin, set.crbegin());

    ASSERT_EQ(crbegin += 1, set.crend());
    ASSERT_EQ(crbegin -= 1, set.crbegin());

    ASSERT_EQ(crbegin + (crend - crbegin), set.crend());
    ASSERT_EQ(crbegin - (crbegin - crend), set.crend());

    ASSERT_EQ(crend - (crend - crbegin), set.crbegin());
    ASSERT_EQ(crend + (crbegin - crend), set.crbegin());

    ASSERT_EQ(crbegin[0u], *set.crbegin().operator->());
    ASSERT_EQ(crbegin[0u], *set.crbegin());

    ASSERT_LT(crbegin, crend);
    ASSERT_LE(crbegin, set.crbegin());

    ASSERT_GT(crend, crbegin);
    ASSERT_GE(crend, set.crend());

    ASSERT_FALSE(set.contains(42));
    set.emplace(42);
    crbegin = set.crbegin();

    ASSERT_EQ(crbegin[0u], 42);
    ASSERT_EQ(crbegin[1u], 13);
}

TEST(SparseSet, Emplace) {
    sparse_set<uint32_t> set;

    ASSERT_FALSE(set.contains(13));
    auto index = set.emplace(13);

    ASSERT_EQ(set.size(), 1);
    ASSERT_TRUE(set.contains(13));
    ASSERT_EQ(index, set.index(13));
    ASSERT_EQ(set[index], 13);
    ASSERT_EQ(*set.cbegin(), 13);

    ASSERT_FALSE(set.contains(42));
    index = set.emplace(42);

    ASSERT_EQ(set.size(), 2);
    ASSERT_TRUE(set.contains(42));
    ASSERT_EQ(index, set.index(42));
    ASSERT_EQ(set[index], 42);
    ASSERT_EQ(*(set.cbegin() + 1), 42);

    ASSERT_FALSE(set.contains(100));
    index = set.emplace(100);

    ASSERT_TRUE(set.contains(100));
    ASSERT_EQ(set.size(), 3);
    ASSERT_EQ(index, set.index(100));
    ASSERT_EQ(set[index], 100);
    ASSERT_EQ(*(set.cbegin() + 2), 100);

    ASSERT_FALSE(set.contains(0));
    index = set.emplace(0);

    ASSERT_EQ(set.size(), 4);
    ASSERT_TRUE(set.contains(0));
    ASSERT_EQ(index, set.index(0));
    ASSERT_EQ(set[index], 0);
    ASSERT_EQ(*(set.cbegin() + 3), 0);
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