#include <cstdint>

#include <gtest/gtest.h>
#include <ecs/entity.hpp>
#include <storage/entity_set.hpp>

using namespace myth::ecs;
using namespace myth::storage;

TEST(EntitySet, Functionalities) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_set<entity_type> set;

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0);
    ASSERT_EQ(set.capacity(), 0);

    entity_type entity1 { 42, 5 };

    ASSERT_FALSE(set.contains(entity1));
    set.emplace(entity1);

    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 1);
    ASSERT_NE(set.capacity(), 0);
    ASSERT_TRUE(set.contains(entity1));
    ASSERT_EQ(set[0], entity1);

    set.clear();

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0);
    ASSERT_NE(set.capacity(), 0);
    ASSERT_FALSE(set.contains(entity1));
}

TEST(EntitySet, Constructors) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_set<entity_type> set1;

    set1 = entity_set<entity_type>{ 10 };

    ASSERT_EQ(set1.size(), 0);
    ASSERT_GE(set1.capacity(), 10);

    entity_type entity1 { 42, 5 };

    ASSERT_FALSE(set1.contains(entity1));
    set1.emplace(entity1);

    entity_set<entity_type> tmp { set1 };
    entity_set<entity_type> set2 { std::move(tmp) };

    ASSERT_EQ(set1.size(), 1);
    ASSERT_TRUE(set1.contains(entity1));
    ASSERT_EQ(set1[0], entity1);
    ASSERT_EQ(set2.size(), 1);
    ASSERT_TRUE(set2.contains(entity1));
    ASSERT_EQ(set2[0], entity1);
}

TEST(EntitySet, Copy) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_set<entity_type> set1;

    entity_type entity1 { 42, 5 };

    ASSERT_FALSE(set1.contains(entity1));
    set1.emplace(entity1);

    entity_set<entity_type> set2 { set1 };

    ASSERT_EQ(set1.size(), 1);
    ASSERT_TRUE(set1.contains(entity1));
    ASSERT_EQ(set1[0], entity1);
    ASSERT_EQ(set2.size(), 1);
    ASSERT_TRUE(set2.contains(entity1));
    ASSERT_EQ(set2[0], entity1);

    ASSERT_FALSE(set1.contains(entity_type { 100, 5 }));
    set1.emplace(entity_type { 100, 5 });
    ASSERT_FALSE(set1.contains(entity_type { 200, 5 }));
    set1.emplace(entity_type { 200, 5 });
    ASSERT_FALSE(set2.contains(entity_type { 300, 5 }));
    set2.emplace(entity_type { 300, 5 });
    set2 = set1;

    ASSERT_EQ(set1.size(), 3);
    ASSERT_TRUE(set1.contains(entity_type { 42, 5 }));
    ASSERT_TRUE(set1.contains(entity_type { 100, 5 }));
    ASSERT_TRUE(set1.contains(entity_type { 200, 5 }));
    ASSERT_EQ(set1[0], (entity_type { 42, 5 }));
    ASSERT_EQ(set1[1], (entity_type { 100, 5 }));
    ASSERT_EQ(set1[2], (entity_type { 200, 5 }));
    ASSERT_EQ(set2.size(), 3);
    ASSERT_TRUE(set2.contains(entity_type { 42, 5 }));
    ASSERT_TRUE(set2.contains(entity_type { 100, 5 }));
    ASSERT_TRUE(set2.contains(entity_type { 200, 5 }));
    ASSERT_EQ(set2[0], (entity_type { 42, 5 }));
    ASSERT_EQ(set2[1], (entity_type { 100, 5 }));
    ASSERT_EQ(set2[2], (entity_type { 200, 5 }));
}

TEST(EntitySet, Move) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_set<entity_type> set1;

    entity_type entity1 { 42, 5 };

    ASSERT_FALSE(set1.contains(entity1));
    set1.emplace(entity1);

    entity_set<entity_type> set2 { std::move(set1) };

    ASSERT_EQ(set1.size(), 0);
    ASSERT_FALSE(set1.contains(entity1));
    ASSERT_EQ(set2.size(), 1);
    ASSERT_TRUE(set2.contains(entity1));
    ASSERT_EQ(set2[0], entity1);

    ASSERT_FALSE(set1.contains(entity_type { 100, 5 }));
    set1.emplace(entity_type { 100, 5 });
    ASSERT_FALSE(set1.contains(entity_type { 200, 5 }));
    set1.emplace(entity_type { 200, 5 });
    ASSERT_FALSE(set2.contains(entity_type { 300, 5 }));
    set2.emplace(entity_type { 300, 5 });
    set2 = std::move(set1);

    ASSERT_EQ(set1.size(), 0);
    ASSERT_FALSE(set1.contains(entity_type { 42, 5 }));
    ASSERT_FALSE(set1.contains(entity_type { 100, 5 }));
    ASSERT_FALSE(set1.contains(entity_type { 200, 5 }));
    ASSERT_EQ(set2.size(), 2);
    ASSERT_TRUE(set2.contains(entity_type { 100, 5 }));
    ASSERT_TRUE(set2.contains(entity_type { 200, 5 }));
    ASSERT_EQ(set2[0], (entity_type { 100, 5 }));
    ASSERT_EQ(set2[1], (entity_type { 200, 5 }));
}

TEST(EntitySet, Emplace) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_set<entity_type> set;

    entity_type entity1 { 42, 5 };

    ASSERT_FALSE(set.contains(entity1));
    size_t index1 = set.emplace(entity1);

    ASSERT_EQ(set.size(), 1);
    ASSERT_TRUE(set.contains(entity1));
    ASSERT_EQ(index1, set.index(entity1));
    ASSERT_EQ(set[index1], entity1);

    entity_type entity2 { 100, 10 };

    ASSERT_FALSE(set.contains(entity2));
    size_t index2 = set.emplace(entity2);

    ASSERT_EQ(set.size(), 2);
    ASSERT_TRUE(set.contains(entity2));
    ASSERT_EQ(index2, set.index(entity2));
    ASSERT_EQ(set[index2], entity2);

    entity_type entity3 { 200, 15 };

    ASSERT_FALSE(set.contains(entity3));
    size_t index3 = set.emplace(entity3);

    ASSERT_EQ(set.size(), 3);
    ASSERT_TRUE(set.contains(entity3));
    ASSERT_EQ(index3, set.index(entity3));
    ASSERT_EQ(set[index3], entity3);

    entity_type entity4 { 300, 20 };

    ASSERT_FALSE(set.contains(entity4));
    size_t index4 = set.emplace(entity4);

    ASSERT_EQ(set.size(), 4);
    ASSERT_TRUE(set.contains(entity4));
    ASSERT_EQ(index4, set.index(entity4));
    ASSERT_EQ(set[index4], entity4);
}

TEST(EntitySet, Erase) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_set<entity_type> set;

    entity_type entity1 { 42, 5 };
    entity_type entity2 { 100, 10 };
    entity_type entity3 { 200, 15 };
    entity_type entity4 { 300, 20 };

    ASSERT_FALSE(set.contains(entity1));
    set.emplace(entity1);
    ASSERT_FALSE(set.contains(entity2));
    set.emplace(entity2);
    ASSERT_FALSE(set.contains(entity3));
    set.emplace(entity3);
    ASSERT_FALSE(set.contains(entity4));
    set.emplace(entity4);

    ASSERT_EQ(set.size(), 4);
    ASSERT_TRUE(set.contains(entity1));
    ASSERT_TRUE(set.contains(entity2));
    ASSERT_TRUE(set.contains(entity3));
    ASSERT_TRUE(set.contains(entity4));
    ASSERT_EQ(set.index(entity1), 0);
    ASSERT_EQ(set.index(entity2), 1);
    ASSERT_EQ(set.index(entity3), 2);
    ASSERT_EQ(set.index(entity4), 3);

    set.erase(entity2);

    ASSERT_EQ(set.size(), 3);
    ASSERT_TRUE(set.contains(entity1));
    ASSERT_FALSE(set.contains(entity2));
    ASSERT_TRUE(set.contains(entity3));
    ASSERT_TRUE(set.contains(entity4));
    ASSERT_EQ(set.index(entity1), 0);
    ASSERT_EQ(set.index(entity4), 1);
    ASSERT_EQ(set.index(entity3), 2);

    set.erase(entity1);

    ASSERT_EQ(set.size(), 2);
    ASSERT_FALSE(set.contains(entity1));
    ASSERT_FALSE(set.contains(entity2));
    ASSERT_TRUE(set.contains(entity3));
    ASSERT_TRUE(set.contains(entity4));
    ASSERT_EQ(set.index(entity3), 0);
    ASSERT_EQ(set.index(entity4), 1);

    set.erase(entity4);

    ASSERT_EQ(set.size(), 1);
    ASSERT_FALSE(set.contains(entity1));
    ASSERT_FALSE(set.contains(entity2));
    ASSERT_TRUE(set.contains(entity3));
    ASSERT_FALSE(set.contains(entity4));
    ASSERT_EQ(set.index(entity3), 0);

    set.erase(entity3);

    ASSERT_TRUE(set.empty());
    ASSERT_FALSE(set.contains(entity1));
    ASSERT_FALSE(set.contains(entity2));
    ASSERT_FALSE(set.contains(entity3));
    ASSERT_FALSE(set.contains(entity4));
}