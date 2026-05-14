#include <cstdint>
#include <limits>

#include <gtest/gtest.h>
#include <ecs/entity.hpp>

using namespace myth::ecs;

TEST(Entity, Construction) {
    basic_entity<uint32_t, uint16_t> entity1;

    ASSERT_EQ(entity1.id(), 0);
    ASSERT_EQ(entity1.version(), 0);
    ASSERT_TRUE(entity1.valid());

    basic_entity<uint32_t, uint16_t> entity2 { 42 };

    ASSERT_EQ(entity2.id(), 42);
    ASSERT_EQ(entity2.version(), 0);
    ASSERT_TRUE(entity2.valid());

    basic_entity<uint32_t, uint16_t> entity3 { 100, 5 };

    ASSERT_EQ(entity3.id(), 100);
    ASSERT_EQ(entity3.version(), 5);
    ASSERT_TRUE(entity3.valid());
}

TEST(Entity, Copy) {
    basic_entity<uint32_t, uint16_t> entity1 { 42, 5 };
    basic_entity<uint32_t, uint16_t> entity2 = entity1;

    ASSERT_EQ(entity1.id(), 42);
    ASSERT_EQ(entity1.version(), 5);
    ASSERT_TRUE(entity1.valid());
    ASSERT_EQ(entity2.id(), 42);
    ASSERT_EQ(entity2.version(), 5);
    ASSERT_TRUE(entity2.valid());
}

TEST(Entity, NullEntity) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    constexpr uint32_t null_id = std::numeric_limits<uint32_t>::max();

    entity_type null_entity(null_id);

    ASSERT_EQ(null_entity.id(), entity_type::null_entity_id);
    ASSERT_EQ(null_entity.version(), 0);
    ASSERT_FALSE(null_entity.valid());
}

TEST(Entity, Equality) {
    basic_entity<uint32_t, uint16_t> entity1 { 42, 5 };
    basic_entity<uint32_t, uint16_t> entity2 { 42, 5 };
    basic_entity<uint32_t, uint16_t> entity3 { 42, 6 };
    basic_entity<uint32_t, uint16_t> entity4 { 43, 5 };

    ASSERT_TRUE(entity1 == entity2);
    ASSERT_FALSE(entity1 != entity2);

    ASSERT_FALSE(entity1 == entity3);
    ASSERT_TRUE(entity1 != entity3);

    ASSERT_FALSE(entity1 == entity4);
    ASSERT_TRUE(entity1 != entity4);
}