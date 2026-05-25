#include <cstdint>

#include <gtest/gtest.h>
#include <ecs/entity.hpp>
#include <storage/entity_storage.hpp>

using namespace myth::ecs;
using namespace myth::storage;

TEST(EntityStorage, Functionalities) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_storage<entity_type, int> storage;

    ASSERT_TRUE(storage.empty());
    ASSERT_EQ(storage.size(), 0);
    ASSERT_EQ(storage.capacity(), 0);

    auto e1 = storage.spawn();

    ASSERT_FALSE(storage.empty());
    ASSERT_EQ(storage.size(), 1);
    ASSERT_NE(storage.capacity(), 0);
    ASSERT_TRUE(storage.contains(e1));
    ASSERT_FALSE(storage.alive(e1));

    storage.emplace(e1, 42);

    ASSERT_EQ(storage[storage.index(e1)], 42);
    ASSERT_EQ(storage[0], 42);
    ASSERT_TRUE(storage.contains(e1));
    ASSERT_TRUE(storage.alive(e1));

    storage.erase(e1);

    ASSERT_TRUE(storage.contains(e1));
    ASSERT_FALSE(storage.alive(e1));

    storage.despawn(e1);

    ASSERT_TRUE(storage.empty());
    ASSERT_EQ(storage.size(), 0);
    ASSERT_FALSE(storage.contains(e1));

    storage.clear();

    ASSERT_TRUE(storage.empty());
    ASSERT_EQ(storage.size(), 0);
}

TEST(EntityStorage, Constructors) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_storage<entity_type, int> storage1;

    storage1 = entity_storage<entity_type, int>{ 10 };

    ASSERT_EQ(storage1.size(), 0);
    ASSERT_GE(storage1.capacity(), 10);

    auto e1 = storage1.spawn();

    storage1.emplace(e1, 42);

    entity_storage<entity_type, int> tmp { storage1 };
    entity_storage<entity_type, int> storage2 { std::move(tmp) };

    ASSERT_EQ(storage1.size(), 1);
    ASSERT_EQ(storage1[storage1.index(e1)], 42);
    ASSERT_EQ(storage1[0], 42);
    ASSERT_EQ(storage2.size(), 1);
    ASSERT_EQ(storage2[storage2.index(e1)], 42);
    ASSERT_EQ(storage2[0], 42);
}

TEST(EntityStorage, Copy) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_storage<entity_type, int> storage1;

    auto e1 = storage1.spawn();
    auto e2 = storage1.spawn();

    storage1.emplace(e1, 10);
    storage1.emplace(e2, 20);

    entity_storage<entity_type, int> storage2 { storage1 };

    ASSERT_EQ(storage1.size(), 2);
    ASSERT_EQ(storage1[storage1.index(e1)], 10);
    ASSERT_EQ(storage1[storage1.index(e2)], 20);
    ASSERT_EQ(storage1[0], 10);
    ASSERT_EQ(storage1[1], 20);
    ASSERT_EQ(storage2.size(), 2);
    ASSERT_EQ(storage2[storage2.index(e1)], 10);
    ASSERT_EQ(storage2[storage2.index(e2)], 20);
    ASSERT_EQ(storage2[0], 10);
    ASSERT_EQ(storage2[1], 20);

    auto e3 = storage1.spawn();
    storage1.emplace(e3, 30);
    auto e4 = storage1.spawn();
    storage1.emplace(e4, 40);
    auto e5 = storage2.spawn();
    storage2.emplace(e5, 50);
    storage2 = storage1;

    ASSERT_EQ(storage1.size(), 4);
    ASSERT_EQ(storage1[storage1.index(e1)], 10);
    ASSERT_EQ(storage1[storage1.index(e2)], 20);
    ASSERT_EQ(storage1[storage1.index(e3)], 30);
    ASSERT_EQ(storage1[storage1.index(e4)], 40);
    ASSERT_EQ(storage2.size(), 4);
    ASSERT_EQ(storage2[storage2.index(e1)], 10);
    ASSERT_EQ(storage2[storage2.index(e2)], 20);
    ASSERT_EQ(storage2[storage2.index(e3)], 30);
    ASSERT_EQ(storage2[storage2.index(e4)], 40);
}

TEST(EntityStorage, Move) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_storage<entity_type, int> storage1;

    auto e1 = storage1.spawn();

    storage1.emplace(e1, 42);

    entity_storage<entity_type, int> storage2 { std::move(storage1) };

    ASSERT_EQ(storage1.size(), 0);
    ASSERT_EQ(storage2.size(), 1);
    ASSERT_EQ(storage2[storage2.index(e1)], 42);
    ASSERT_EQ(storage2[0], 42);

    auto e2 = storage1.spawn();
    storage1.emplace(e2, 10);
    auto e3 = storage1.spawn();
    storage1.emplace(e3, 20);
    auto e4 = storage2.spawn();
    storage2.emplace(e4, 30);
    storage2 = std::move(storage1);

    ASSERT_EQ(storage1.size(), 0);
    ASSERT_EQ(storage2.size(), 2);
    ASSERT_EQ(storage2[storage2.index(e2)], 10);
    ASSERT_EQ(storage2[storage2.index(e3)], 20);
    ASSERT_EQ(storage2[0], 10);
    ASSERT_EQ(storage2[1], 20);
}

TEST(EntityStorage, Spawn) {
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

    storage.despawn(e2);

    ASSERT_EQ(storage.size(), 2);
    ASSERT_FALSE(storage.contains(e2));

    auto e4 = storage.spawn();

    ASSERT_EQ(storage.size(), 3);
    ASSERT_EQ(e4.id(), e2.id());
    ASSERT_GT(e4.version(), e2.version());
    ASSERT_EQ(e4, (entity_type{ 1, 1 }));

    storage.despawn(e1);

    ASSERT_EQ(storage.size(), 2);

    auto e5 = storage.spawn();

    ASSERT_EQ(storage.size(), 3);
    ASSERT_EQ(e5.id(), e1.id());
    ASSERT_GT(e5.version(), e1.version());
    ASSERT_EQ(e5, (entity_type{ 0, 1 }));

    storage.despawn(e3);
    storage.despawn(e4);
    storage.despawn(e5);

    ASSERT_TRUE(storage.empty());
    ASSERT_EQ(storage.size(), 0);
}

TEST(EntityStorage, Emplace) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_storage<entity_type, int> storage;

    auto e1 = storage.spawn();
    auto e2 = storage.spawn();

    storage.emplace(e1, 10);

    ASSERT_EQ(storage[storage.index(e1)], 10);
    ASSERT_EQ(storage[0], 10);
    ASSERT_TRUE(storage.alive(e1));

    storage.emplace(e2, 20);

    ASSERT_EQ(storage[storage.index(e2)], 20);
    ASSERT_EQ(storage[1], 20);
    ASSERT_TRUE(storage.alive(e2));

    auto e3 = storage.spawn();

    ASSERT_FALSE(storage.alive(e3));

    storage.emplace(e3, 30);

    ASSERT_EQ(storage[storage.index(e3)], 30);
    ASSERT_EQ(storage[2], 30);
    ASSERT_TRUE(storage.alive(e3));

    auto e4 = storage.spawn();

    ASSERT_FALSE(storage.alive(e4));

    storage.emplace(e4, 40);

    ASSERT_EQ(storage[storage.index(e4)], 40);
    ASSERT_EQ(storage[3], 40);
    ASSERT_TRUE(storage.alive(e4));

    ASSERT_EQ(storage[storage.index(e1)], 10);
    ASSERT_EQ(storage[storage.index(e2)], 20);
    ASSERT_EQ(storage[storage.index(e3)], 30);
    ASSERT_EQ(storage[storage.index(e4)], 40);

    ASSERT_EQ(storage.size(), 4);
}

TEST(EntityStorage, Erase) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_storage<entity_type, int> storage;

    auto e1 = storage.spawn();
    auto e2 = storage.spawn();
    auto e3 = storage.spawn();
    auto e4 = storage.spawn();

    storage.emplace(e1, 10);
    storage.emplace(e2, 20);
    storage.emplace(e3, 30);
    storage.emplace(e4, 40);

    ASSERT_EQ(storage.size(), 4);
    ASSERT_EQ(storage[storage.index(e1)], 10);
    ASSERT_EQ(storage[storage.index(e2)], 20);
    ASSERT_EQ(storage[storage.index(e3)], 30);
    ASSERT_EQ(storage[storage.index(e4)], 40);
    ASSERT_EQ(storage[0], 10);
    ASSERT_EQ(storage[1], 20);
    ASSERT_EQ(storage[2], 30);
    ASSERT_EQ(storage[3], 40);
    ASSERT_TRUE(storage.alive(e1));
    ASSERT_TRUE(storage.alive(e2));
    ASSERT_TRUE(storage.alive(e3));
    ASSERT_TRUE(storage.alive(e4));

    storage.erase(e2);

    ASSERT_EQ(storage.size(), 4);
    ASSERT_EQ(storage[storage.index(e1)], 10);
    ASSERT_EQ(storage[storage.index(e3)], 30);
    ASSERT_EQ(storage[storage.index(e4)], 40);
    ASSERT_EQ(storage[0], 10);
    ASSERT_EQ(storage[1], 40);
    ASSERT_EQ(storage[2], 30);
    ASSERT_TRUE(storage.alive(e1));
    ASSERT_FALSE(storage.alive(e2));
    ASSERT_TRUE(storage.alive(e3));
    ASSERT_TRUE(storage.alive(e4));

    storage.erase(e1);

    ASSERT_EQ(storage.size(), 4);
    ASSERT_EQ(storage[storage.index(e3)], 30);
    ASSERT_EQ(storage[storage.index(e4)], 40);
    ASSERT_EQ(storage[0], 30);
    ASSERT_EQ(storage[1], 40);
    ASSERT_FALSE(storage.alive(e1));
    ASSERT_TRUE(storage.alive(e3));
    ASSERT_TRUE(storage.alive(e4));

    storage.erase(e4);

    ASSERT_EQ(storage.size(), 4);
    ASSERT_EQ(storage[storage.index(e3)], 30);
    ASSERT_EQ(storage[0], 30);
    ASSERT_TRUE(storage.alive(e3));
    ASSERT_FALSE(storage.alive(e4));

    storage.erase(e3);

    ASSERT_EQ(storage.size(), 4);
    ASSERT_FALSE(storage.alive(e1));
    ASSERT_FALSE(storage.alive(e2));
    ASSERT_FALSE(storage.alive(e3));
    ASSERT_FALSE(storage.alive(e4));
}
