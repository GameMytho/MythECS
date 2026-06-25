#include <cstdint>

#include <gtest/gtest.h>
#include <ecs/entity.hpp>
#include <storage/entity_storage.hpp>

using namespace myth::ecs;
using namespace myth::storage;

// ============================================================================
// Functionalities - spawn, emplace, erase, despawn, clear lifecycle
// ============================================================================
TEST(EntityStorage, Functionalities) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_storage<entity_type, int> storage;

    // Empty on construction.
    ASSERT_TRUE(storage.empty());
    ASSERT_EQ(storage.size(), 0);
    ASSERT_EQ(storage.capacity(), 0);

    // Spawn an entity - it exists (contains) but has no value yet (not alive).
    auto e1 = storage.spawn();

    ASSERT_FALSE(storage.empty());
    ASSERT_EQ(storage.size(), 1);
    ASSERT_NE(storage.capacity(), 0);
    ASSERT_TRUE(storage.contains(e1));
    ASSERT_FALSE(storage.alive(e1));
    ASSERT_EQ(storage.index(e1), 0);
    ASSERT_EQ(storage.checked_index(e1), 0);

    // Emplace a component value - now the entity is alive.
    storage.emplace(e1, 42);

    ASSERT_TRUE(storage.contains(e1));
    ASSERT_TRUE(storage.alive(e1));
    ASSERT_EQ(storage.index(e1), 0);
    ASSERT_EQ(storage.checked_index(e1), 0);
    ASSERT_EQ(storage[0], 42);

    // Erase the value - entity still exists but no longer alive.
    storage.erase(e1);

    ASSERT_TRUE(storage.contains(e1));
    ASSERT_FALSE(storage.alive(e1));
    ASSERT_EQ(storage.index(e1), 0);
    ASSERT_EQ(storage.checked_index(e1), 0);

    // Despawn removes the entity entirely.
    storage.despawn(e1);

    ASSERT_TRUE(storage.empty());
    ASSERT_EQ(storage.size(), 0);
    ASSERT_FALSE(storage.contains(e1));
    ASSERT_EQ(storage.checked_index(e1), (entity_storage<entity_type, int>::null_entity_index));

    // Clear on already-empty storage is safe.
    storage.clear();

    ASSERT_TRUE(storage.empty());
    ASSERT_EQ(storage.size(), 0);
    ASSERT_EQ(storage.checked_index(e1), (entity_storage<entity_type, int>::null_entity_index));
}

// ============================================================================
// Constructors - default, capacity, copy, move
// ============================================================================
TEST(EntityStorage, Constructors) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_storage<entity_type, int> storage1;

    storage1 = entity_storage<entity_type, int>{ 10 };

    ASSERT_EQ(storage1.size(), 0);
    ASSERT_GE(storage1.capacity(), 10);

    auto e1 = storage1.spawn();

    storage1.emplace(e1, 42);

    // Copy-then-move construction chain.
    entity_storage<entity_type, int> tmp { storage1 };
    entity_storage<entity_type, int> storage2 { std::move(tmp) };

    ASSERT_EQ(storage1.size(), 1);
    ASSERT_TRUE(storage1.contains(e1));
    ASSERT_TRUE(storage1.alive(e1));
    ASSERT_EQ(storage1.index(e1), 0);
    ASSERT_EQ(storage1.checked_index(e1), 0);
    ASSERT_EQ(storage1[0], 42);

    ASSERT_EQ(storage2.size(), 1);
    ASSERT_TRUE(storage2.contains(e1));
    ASSERT_TRUE(storage2.alive(e1));
    ASSERT_EQ(storage2.index(e1), 0);
    ASSERT_EQ(storage2.checked_index(e1), 0);
    ASSERT_EQ(storage2[0], 42);
}

// ============================================================================
// Copy - copy ctor and copy assignment; storages are independent
// ============================================================================
TEST(EntityStorage, Copy) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_storage<entity_type, int> storage1;

    auto e1 = storage1.spawn();
    auto e2 = storage1.spawn();

    storage1.emplace(e1, 10);
    storage1.emplace(e2, 20);

    // Copy ctor - storage2 is independent.
    entity_storage<entity_type, int> storage2 { storage1 };

    ASSERT_EQ(storage1.size(), 2);
    ASSERT_TRUE(storage1.contains(e1));
    ASSERT_TRUE(storage1.contains(e2));
    ASSERT_TRUE(storage1.alive(e1));
    ASSERT_TRUE(storage1.alive(e2));
    ASSERT_EQ(storage1.index(e1), 0);
    ASSERT_EQ(storage1.index(e2), 1);
    ASSERT_EQ(storage1.checked_index(e1), 0);
    ASSERT_EQ(storage1.checked_index(e2), 1);
    ASSERT_EQ(storage1[0], 10);
    ASSERT_EQ(storage1[1], 20);

    ASSERT_EQ(storage2.size(), 2);
    ASSERT_TRUE(storage2.contains(e1));
    ASSERT_TRUE(storage2.contains(e2));
    ASSERT_TRUE(storage2.alive(e1));
    ASSERT_TRUE(storage2.alive(e2));
    ASSERT_EQ(storage2.index(e1), 0);
    ASSERT_EQ(storage2.index(e2), 1);
    ASSERT_EQ(storage2.checked_index(e1), 0);
    ASSERT_EQ(storage2.checked_index(e2), 1);
    ASSERT_EQ(storage2[0], 10);
    ASSERT_EQ(storage2[1], 20);

    // Mutate independently.
    auto e3 = storage1.spawn();
    storage1.emplace(e3, 30);
    auto e4 = storage1.spawn();
    storage1.emplace(e4, 40);
    auto e5 = storage2.spawn();
    storage2.emplace(e5, 50);

    // Copy assignment - storage2 replaced by storage1's contents.
    storage2 = storage1;

    ASSERT_EQ(storage1.size(), 4);
    ASSERT_TRUE(storage1.contains(e1));
    ASSERT_TRUE(storage1.contains(e2));
    ASSERT_TRUE(storage1.contains(e3));
    ASSERT_TRUE(storage1.contains(e4));
    ASSERT_TRUE(storage1.alive(e1));
    ASSERT_TRUE(storage1.alive(e2));
    ASSERT_TRUE(storage1.alive(e3));
    ASSERT_TRUE(storage1.alive(e4));
    ASSERT_EQ(storage1.index(e1), 0);
    ASSERT_EQ(storage1.index(e2), 1);
    ASSERT_EQ(storage1.index(e3), 2);
    ASSERT_EQ(storage1.index(e4), 3);
    ASSERT_EQ(storage1.checked_index(e1), 0);
    ASSERT_EQ(storage1.checked_index(e2), 1);
    ASSERT_EQ(storage1.checked_index(e3), 2);
    ASSERT_EQ(storage1.checked_index(e4), 3);
    ASSERT_EQ(storage1[0], 10);
    ASSERT_EQ(storage1[1], 20);
    ASSERT_EQ(storage1[2], 30);
    ASSERT_EQ(storage1[3], 40);

    ASSERT_EQ(storage2.size(), 4);
    ASSERT_TRUE(storage2.contains(e1));
    ASSERT_TRUE(storage2.contains(e2));
    ASSERT_TRUE(storage2.contains(e3));
    ASSERT_TRUE(storage2.contains(e4));
    ASSERT_TRUE(storage2.alive(e1));
    ASSERT_TRUE(storage2.alive(e2));
    ASSERT_TRUE(storage2.alive(e3));
    ASSERT_TRUE(storage2.alive(e4));
    ASSERT_EQ(storage2.index(e1), 0);
    ASSERT_EQ(storage2.index(e2), 1);
    ASSERT_EQ(storage2.index(e3), 2);
    ASSERT_EQ(storage2.index(e4), 3);
    ASSERT_EQ(storage2.checked_index(e1), 0);
    ASSERT_EQ(storage2.checked_index(e2), 1);
    ASSERT_EQ(storage2.checked_index(e3), 2);
    ASSERT_EQ(storage2.checked_index(e4), 3);
    ASSERT_EQ(storage2[0], 10);
    ASSERT_EQ(storage2[1], 20);
    ASSERT_EQ(storage2[2], 30);
    ASSERT_EQ(storage2[3], 40);
}

// ============================================================================
// Move - move ctor and move assignment; source left empty
// ============================================================================
TEST(EntityStorage, Move) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_storage<entity_type, int> storage1;

    auto e1 = storage1.spawn();

    storage1.emplace(e1, 42);

    // Move ctor.
    entity_storage<entity_type, int> storage2 { std::move(storage1) };

    ASSERT_EQ(storage1.size(), 0);

    ASSERT_EQ(storage2.size(), 1);
    ASSERT_TRUE(storage2.contains(e1));
    ASSERT_TRUE(storage2.alive(e1));
    ASSERT_EQ(storage2.index(e1), 0);
    ASSERT_EQ(storage2.checked_index(e1), 0);
    ASSERT_EQ(storage2[0], 42);

    // Moved-from storage can be reused.
    auto e2 = storage1.spawn();
    storage1.emplace(e2, 10);
    auto e3 = storage1.spawn();
    storage1.emplace(e3, 20);
    auto e4 = storage2.spawn();
    storage2.emplace(e4, 30);

    // Move assignment.
    storage2 = std::move(storage1);

    ASSERT_EQ(storage1.size(), 0);

    ASSERT_EQ(storage2.size(), 2);
    ASSERT_TRUE(storage2.contains(e2));
    ASSERT_TRUE(storage2.contains(e3));
    ASSERT_TRUE(storage2.alive(e2));
    ASSERT_TRUE(storage2.alive(e3));
    ASSERT_EQ(storage2.index(e2), 0);
    ASSERT_EQ(storage2.index(e3), 1);
    ASSERT_EQ(storage2.checked_index(e2), 0);
    ASSERT_EQ(storage2.checked_index(e3), 1);
    ASSERT_EQ(storage2[0], 10);
    ASSERT_EQ(storage2[1], 20);
}

// ============================================================================
// Spawn & Despawn - entity lifecycle with version bumping on id reuse
// ============================================================================
TEST(EntityStorage, SpawnAndDespawn) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_storage<entity_type, int> storage;

    auto e1 = storage.spawn();
    auto e2 = storage.spawn();
    auto e3 = storage.spawn();

    ASSERT_EQ(storage.size(), 3);
    ASSERT_EQ(e1, (entity_type{ 0, 0 }));
    ASSERT_EQ(e2, (entity_type{ 1, 0 }));
    ASSERT_EQ(e3, (entity_type{ 2, 0 }));
    ASSERT_TRUE(storage.contains(e1));
    ASSERT_TRUE(storage.contains(e2));
    ASSERT_TRUE(storage.contains(e3));
    ASSERT_FALSE(storage.alive(e1));
    ASSERT_FALSE(storage.alive(e2));
    ASSERT_FALSE(storage.alive(e3));
    ASSERT_EQ(storage.index(e1), 0);
    ASSERT_EQ(storage.index(e2), 1);
    ASSERT_EQ(storage.index(e3), 2);
    ASSERT_EQ(storage.checked_index(e1), 0);
    ASSERT_EQ(storage.checked_index(e2), 1);
    ASSERT_EQ(storage.checked_index(e3), 2);

    // Despawn e2 - its id (1) is now free for reuse.
    storage.despawn(e2);

    ASSERT_EQ(storage.size(), 2);
    ASSERT_FALSE(storage.contains(e2));
    ASSERT_FALSE(storage.alive(e2));
    ASSERT_EQ(storage.checked_index(e2), (entity_storage<entity_type, int>::null_entity_index));

    // Spawn reuses the freed id (1) with version bumped to 1.
    auto e4 = storage.spawn();

    ASSERT_TRUE(storage.contains(e4));
    ASSERT_FALSE(storage.alive(e4));
    ASSERT_EQ(storage.index(e4), 2);
    ASSERT_EQ(storage.checked_index(e4), 2);
    ASSERT_EQ(storage.size(), 3);
    ASSERT_EQ(e4.id(), e2.id());
    ASSERT_GT(e4.version(), e2.version());
    ASSERT_EQ(e4, (entity_type{ 1, 1 }));

    // Old entity handle for id=1,v=0 is stale - unsafe index same, safe index rejects.
    ASSERT_EQ(storage.index(e2), storage.index(e4));
    ASSERT_EQ(storage.checked_index(e2), (entity_storage<entity_type, int>::null_entity_index));

    // Despawn e1, then respawn - id 0 reused with version 1.
    storage.despawn(e1);

    ASSERT_EQ(storage.size(), 2);
    ASSERT_FALSE(storage.contains(e1));
    ASSERT_FALSE(storage.alive(e1));
    ASSERT_EQ(storage.checked_index(e1), (entity_storage<entity_type, int>::null_entity_index));

    auto e5 = storage.spawn();

    ASSERT_TRUE(storage.contains(e5));
    ASSERT_FALSE(storage.alive(e5));
    ASSERT_EQ(storage.index(e5), 2);
    ASSERT_EQ(storage.checked_index(e5), 2);
    ASSERT_EQ(storage.size(), 3);
    ASSERT_EQ(e5.id(), e1.id());
    ASSERT_GT(e5.version(), e1.version());
    ASSERT_EQ(e5, (entity_type{ 0, 1 }));

    ASSERT_EQ(storage.index(e1), storage.index(e5));
    ASSERT_EQ(storage.checked_index(e1), (entity_storage<entity_type, int>::null_entity_index));

    // Despawn all remaining entities - storage becomes empty.
    storage.despawn(e3);
    storage.despawn(e4);
    storage.despawn(e5);

    ASSERT_TRUE(storage.empty());
    ASSERT_EQ(storage.size(), 0);
    ASSERT_FALSE(storage.contains(e1));
    ASSERT_FALSE(storage.contains(e2));
    ASSERT_FALSE(storage.contains(e3));
    ASSERT_FALSE(storage.contains(e4));
    ASSERT_FALSE(storage.contains(e5));
    ASSERT_FALSE(storage.alive(e1));
    ASSERT_FALSE(storage.alive(e2));
    ASSERT_FALSE(storage.alive(e3));
    ASSERT_FALSE(storage.alive(e4));
    ASSERT_FALSE(storage.alive(e5));
    ASSERT_EQ(storage.checked_index(e1), (entity_storage<entity_type, int>::null_entity_index));
    ASSERT_EQ(storage.checked_index(e2), (entity_storage<entity_type, int>::null_entity_index));
    ASSERT_EQ(storage.checked_index(e3), (entity_storage<entity_type, int>::null_entity_index));
    ASSERT_EQ(storage.checked_index(e4), (entity_storage<entity_type, int>::null_entity_index));
    ASSERT_EQ(storage.checked_index(e5), (entity_storage<entity_type, int>::null_entity_index));
}

// ============================================================================
// Emplace - assign component values to spawned entities
// ============================================================================
TEST(EntityStorage, Emplace) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_storage<entity_type, int> storage;

    auto e1 = storage.spawn();
    auto e2 = storage.spawn();

    // Spawned entities exist but are not yet alive.
    ASSERT_TRUE(storage.contains(e1));
    ASSERT_TRUE(storage.contains(e2));
    ASSERT_FALSE(storage.alive(e1));
    ASSERT_FALSE(storage.alive(e2));
    ASSERT_EQ(storage.size(), 2);

    storage.emplace(e1, 10);

    ASSERT_TRUE(storage.contains(e1));
    ASSERT_TRUE(storage.alive(e1));
    ASSERT_EQ(storage.index(e1), 0);
    ASSERT_EQ(storage.checked_index(e1), 0);
    ASSERT_EQ(storage[0], 10);

    storage.emplace(e2, 20);

    ASSERT_TRUE(storage.contains(e2));
    ASSERT_TRUE(storage.alive(e2));
    ASSERT_EQ(storage.index(e2), 1);
    ASSERT_EQ(storage.checked_index(e2), 1);
    ASSERT_EQ(storage[1], 20);

    // Spawn a third, emplace, and verify packed layout.
    auto e3 = storage.spawn();

    ASSERT_TRUE(storage.contains(e3));
    ASSERT_FALSE(storage.alive(e3));
    ASSERT_EQ(storage.size(), 3);

    storage.emplace(e3, 30);

    ASSERT_TRUE(storage.contains(e3));
    ASSERT_TRUE(storage.alive(e3));
    ASSERT_EQ(storage.index(e3), 2);
    ASSERT_EQ(storage.checked_index(e3), 2);
    ASSERT_EQ(storage[2], 30);

    auto e4 = storage.spawn();

    ASSERT_TRUE(storage.contains(e4));
    ASSERT_FALSE(storage.alive(e4));
    ASSERT_EQ(storage.size(), 4);

    storage.emplace(e4, 40);

    ASSERT_TRUE(storage.contains(e4));
    ASSERT_TRUE(storage.alive(e4));
    ASSERT_EQ(storage.index(e4), 3);
    ASSERT_EQ(storage.checked_index(e4), 3);
    ASSERT_EQ(storage[3], 40);
}

// ============================================================================
// Erase - remove component values; entity stays spawned, values compacted
// ============================================================================
TEST(EntityStorage, Erase) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_storage<entity_type, int> storage;

    auto e1 = storage.spawn();
    auto e2 = storage.spawn();
    auto e3 = storage.spawn();
    auto e4 = storage.spawn();

    ASSERT_TRUE(storage.contains(e1));
    ASSERT_TRUE(storage.contains(e2));
    ASSERT_TRUE(storage.contains(e3));
    ASSERT_TRUE(storage.contains(e4));
    ASSERT_FALSE(storage.alive(e1));
    ASSERT_FALSE(storage.alive(e2));
    ASSERT_FALSE(storage.alive(e3));
    ASSERT_FALSE(storage.alive(e4));

    storage.emplace(e1, 10);
    storage.emplace(e2, 20);
    storage.emplace(e3, 30);
    storage.emplace(e4, 40);

    // All four entities alive with values in packed order.
    ASSERT_EQ(storage.size(), 4);
    ASSERT_TRUE(storage.contains(e1));
    ASSERT_TRUE(storage.contains(e2));
    ASSERT_TRUE(storage.contains(e3));
    ASSERT_TRUE(storage.contains(e4));
    ASSERT_TRUE(storage.alive(e1));
    ASSERT_TRUE(storage.alive(e2));
    ASSERT_TRUE(storage.alive(e3));
    ASSERT_TRUE(storage.alive(e4));
    ASSERT_EQ(storage.index(e1), 0);
    ASSERT_EQ(storage.index(e2), 1);
    ASSERT_EQ(storage.index(e3), 2);
    ASSERT_EQ(storage.index(e4), 3);
    ASSERT_EQ(storage.checked_index(e1), 0);
    ASSERT_EQ(storage.checked_index(e2), 1);
    ASSERT_EQ(storage.checked_index(e3), 2);
    ASSERT_EQ(storage.checked_index(e4), 3);
    ASSERT_EQ(storage[0], 10);
    ASSERT_EQ(storage[1], 20);
    ASSERT_EQ(storage[2], 30);
    ASSERT_EQ(storage[3], 40);

    // Erase e2's value - entity still exists, value compacted.
    // Alive zone [0, 4) shrinks to [0, 3); e2 moves to non-alive zone [3, 4).
    storage.erase(e2);

    ASSERT_EQ(storage.size(), 4);
    ASSERT_TRUE(storage.contains(e1));
    ASSERT_TRUE(storage.contains(e2));
    ASSERT_TRUE(storage.contains(e3));
    ASSERT_TRUE(storage.contains(e4));
    ASSERT_TRUE(storage.alive(e1));
    ASSERT_FALSE(storage.alive(e2));
    ASSERT_TRUE(storage.alive(e3));
    ASSERT_TRUE(storage.alive(e4));
    ASSERT_EQ(storage.index(e1), 0);
    ASSERT_EQ(storage.index(e3), 2);
    ASSERT_EQ(storage.index(e4), 1);           // e4 moved to e2's old value slot
    ASSERT_EQ(storage.checked_index(e1), 0);
    ASSERT_EQ(storage.checked_index(e2), 3);   // e2 swapped to non-alive zone
    ASSERT_EQ(storage.checked_index(e3), 2);
    ASSERT_EQ(storage.checked_index(e4), 1);
    ASSERT_EQ(storage[0], 10);
    ASSERT_EQ(storage[1], 40);                 // e4's value now at index 1
    ASSERT_EQ(storage[2], 30);

    // Erase e1.
    storage.erase(e1);

    ASSERT_EQ(storage.size(), 4);
    ASSERT_TRUE(storage.contains(e1));
    ASSERT_TRUE(storage.contains(e2));
    ASSERT_TRUE(storage.contains(e3));
    ASSERT_TRUE(storage.contains(e4));
    ASSERT_FALSE(storage.alive(e1));
    ASSERT_FALSE(storage.alive(e2));
    ASSERT_TRUE(storage.alive(e3));
    ASSERT_TRUE(storage.alive(e4));
    ASSERT_EQ(storage.index(e3), 0);           // e3 moved to e1's old value slot
    ASSERT_EQ(storage.index(e4), 1);
    ASSERT_EQ(storage.checked_index(e1), 2);   // e1 swapped to non-alive zone
    ASSERT_EQ(storage.checked_index(e2), 3);
    ASSERT_EQ(storage.checked_index(e3), 0);
    ASSERT_EQ(storage.checked_index(e4), 1);
    ASSERT_EQ(storage[0], 30);
    ASSERT_EQ(storage[1], 40);

    // Erase e4.
    storage.erase(e4);

    ASSERT_EQ(storage.size(), 4);
    ASSERT_TRUE(storage.contains(e1));
    ASSERT_TRUE(storage.contains(e2));
    ASSERT_TRUE(storage.contains(e3));
    ASSERT_TRUE(storage.contains(e4));
    ASSERT_FALSE(storage.alive(e1));
    ASSERT_FALSE(storage.alive(e2));
    ASSERT_TRUE(storage.alive(e3));
    ASSERT_FALSE(storage.alive(e4));
    ASSERT_EQ(storage.index(e3), 0);
    ASSERT_EQ(storage.checked_index(e1), 2);
    ASSERT_EQ(storage.checked_index(e2), 3);
    ASSERT_EQ(storage.checked_index(e3), 0);
    ASSERT_EQ(storage.checked_index(e4), 1);
    ASSERT_EQ(storage[0], 30);
    ASSERT_FALSE(storage.alive(e1));
    ASSERT_FALSE(storage.alive(e2));
    ASSERT_TRUE(storage.alive(e3));
    ASSERT_FALSE(storage.alive(e4));

    // Erase e3 - all values gone, all entities remain spawned.
    storage.erase(e3);

    ASSERT_EQ(storage.size(), 4);
    ASSERT_TRUE(storage.contains(e1));
    ASSERT_TRUE(storage.contains(e2));
    ASSERT_TRUE(storage.contains(e3));
    ASSERT_TRUE(storage.contains(e4));
    ASSERT_FALSE(storage.alive(e1));
    ASSERT_FALSE(storage.alive(e2));
    ASSERT_FALSE(storage.alive(e3));
    ASSERT_FALSE(storage.alive(e4));
    ASSERT_EQ(storage.checked_index(e1), 2);
    ASSERT_EQ(storage.checked_index(e2), 3);
    ASSERT_EQ(storage.checked_index(e3), 0);
    ASSERT_EQ(storage.checked_index(e4), 1);
    ASSERT_FALSE(storage.alive(e1));
    ASSERT_FALSE(storage.alive(e2));
    ASSERT_FALSE(storage.alive(e3));
    ASSERT_FALSE(storage.alive(e4));
}
