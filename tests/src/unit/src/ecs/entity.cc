#include <cstdint>
#include <limits>

#include <gtest/gtest.h>
#include <ecs/entity.hpp>

using namespace myth::ecs;

// ============================================================================
// Construction - default (null), id-only, id+version
// ============================================================================
TEST(Entity, Construction) {
    basic_entity<uint32_t, uint16_t> entity1;

    // Default-constructed entity has null id and is invalid.
    ASSERT_EQ(entity1.id(), (basic_entity<uint32_t, uint16_t>::null_entity_id));
    ASSERT_EQ(entity1.version(), 0);
    ASSERT_FALSE(entity1.valid());

    // Construct with id only - version defaults to 0.
    basic_entity<uint32_t, uint16_t> entity2 { 42 };

    ASSERT_EQ(entity2.id(), 42);
    ASSERT_EQ(entity2.version(), 0);
    ASSERT_TRUE(entity2.valid());

    // Construct with explicit id and version.
    basic_entity<uint32_t, uint16_t> entity3 { 100, 5 };

    ASSERT_EQ(entity3.id(), 100);
    ASSERT_EQ(entity3.version(), 5);
    ASSERT_TRUE(entity3.valid());
}

// ============================================================================
// Copy - copy ctor and copy assignment
// ============================================================================
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

// ============================================================================
// Null entity - sentinel id = numeric_limits::max(), always invalid
// ============================================================================
TEST(Entity, NullEntity) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    constexpr uint32_t null_id = std::numeric_limits<uint32_t>::max();

    entity_type null_entity;

    ASSERT_EQ(null_entity.id(), null_id);
    ASSERT_EQ(null_entity.version(), 0);
    ASSERT_FALSE(null_entity.valid());
}

// ============================================================================
// Equality - two entities equal iff both id and version match
// ============================================================================
TEST(Entity, Equality) {
    basic_entity<uint32_t, uint16_t> entity1 { 42, 5 };
    basic_entity<uint32_t, uint16_t> entity2 { 42, 5 };
    basic_entity<uint32_t, uint16_t> entity3 { 42, 6 };   // same id, different version
    basic_entity<uint32_t, uint16_t> entity4 { 43, 5 };   // different id, same version

    // Same id + version: equal.
    ASSERT_TRUE(entity1 == entity2);
    ASSERT_FALSE(entity1 != entity2);

    // Different version: not equal (stale reference).
    ASSERT_FALSE(entity1 == entity3);
    ASSERT_TRUE(entity1 != entity3);

    // Different id: not equal.
    ASSERT_FALSE(entity1 == entity4);
    ASSERT_TRUE(entity1 != entity4);
}

// ============================================================================
// Constexpr - construction, copy, move, and comparison all compile-time safe
// ============================================================================
TEST(Entity, Constexpr) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    // Default null entity.
    constexpr entity_type null_entity;

    static_assert(null_entity.id() == entity_type::null_entity_id);
    static_assert(null_entity.version() == 0);
    static_assert(!null_entity.valid());

    // Value construction.
    constexpr entity_type entity1 { 42, 5 };

    static_assert(entity1.id() == 42);
    static_assert(entity1.version() == 5);
    static_assert(entity1.valid());
    static_assert(!(entity1 == null_entity));
    static_assert(entity1 != null_entity);

    // Copy ctor.
    constexpr entity_type entity2 { entity1 };

    static_assert(entity2.id() == 42);
    static_assert(entity2.version() == 5);
    static_assert(entity2.valid());
    static_assert(entity2 == entity1);
    static_assert(!(entity2 != entity1));

    // Copy assignment.
    constexpr entity_type entity3 = entity1;

    static_assert(entity3.id() == 42);
    static_assert(entity3.version() == 5);
    static_assert(entity3.valid());
    static_assert(entity3 == entity1);
    static_assert(!(entity3 != entity1));

    // Move ctor (move = copy for trivial entity).
    constexpr entity_type entity4 { std::move(entity1) };

    static_assert(entity4.id() == 42);
    static_assert(entity4.version() == 5);
    static_assert(entity4.valid());
    static_assert(entity4 == entity1);
    static_assert(!(entity4 != entity1));

    // Move assignment.
    constexpr entity_type entity5 = std::move(entity1);

    static_assert(entity5.id() == 42);
    static_assert(entity5.version() == 5);
    static_assert(entity5.valid());
    static_assert(entity5 == entity1);
    static_assert(!(entity5 != entity1));
}
