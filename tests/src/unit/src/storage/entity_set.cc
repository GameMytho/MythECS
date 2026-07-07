#include <cstdint>

#include <gtest/gtest.h>
#include <ecs/entity.hpp>
#include <storage/entity_set.hpp>

using namespace myth::ecs;
using namespace myth::storage;

// ============================================================================
// Functionalities - empty, emplace, contains, occupied, index, checked_index, clear
// ============================================================================
TEST(EntitySet, Functionalities) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_set<entity_type> set;

    // Default-constructed set is empty.
    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0);
    ASSERT_EQ(set.capacity(), 0);

    entity_type entity1 { 42, 5 };

    // Not yet inserted: neither occupied (by id) nor contains (by id+version).
    ASSERT_FALSE(set.occupied(entity1.id()));
    ASSERT_FALSE(set.contains(entity1));
    ASSERT_EQ(set.checked_index(entity1), entity_set<entity_type>::null_entity_index);
    set.emplace_back(entity1);

    // After emplace: non-empty, allocated, reachable via all accessors.
    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 1);
    ASSERT_NE(set.capacity(), 0);
    ASSERT_TRUE(set.occupied(entity1.id()));
    ASSERT_TRUE(set.contains(entity1));
    ASSERT_EQ(set.index(entity1), 0);
    ASSERT_EQ(set.checked_index(entity1), 0);
    ASSERT_EQ(set[0], entity1);

    // Clear resets size but preserves capacity.
    set.clear();

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0);
    ASSERT_NE(set.capacity(), 0);
    ASSERT_FALSE(set.occupied(entity1.id()));
    ASSERT_FALSE(set.contains(entity1));
    ASSERT_EQ(set.checked_index(entity1), entity_set<entity_type>::null_entity_index);
}

// ============================================================================
// Constructors - default, capacity, copy, move
// ============================================================================
TEST(EntitySet, Constructors) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_set<entity_type> set1;

    set1 = entity_set<entity_type>{ 10 };

    ASSERT_EQ(set1.size(), 0);
    ASSERT_GE(set1.capacity(), 10);

    entity_type entity1 { 42, 5 };

    ASSERT_FALSE(set1.contains(entity1));
    ASSERT_EQ(set1.checked_index(entity1), (entity_set<entity_type>::null_entity_index));
    set1.emplace_back(entity1);

    // Copy-then-move construction chain.
    entity_set<entity_type> tmp { set1 };
    entity_set<entity_type> set2 { std::move(tmp) };

    ASSERT_EQ(set1.size(), 1);
    ASSERT_TRUE(set1.contains(entity1));
    ASSERT_EQ(set1.index(entity1), 0);
    ASSERT_EQ(set1.checked_index(entity1), 0);
    ASSERT_EQ(set1[0], entity1);

    ASSERT_EQ(set2.size(), 1);
    ASSERT_TRUE(set2.contains(entity1));
    ASSERT_EQ(set2.index(entity1), 0);
    ASSERT_EQ(set2.checked_index(entity1), 0);
    ASSERT_EQ(set2[0], entity1);
}

// ============================================================================
// Copy - copy ctor and copy assignment; sets are independent
// ============================================================================
TEST(EntitySet, Copy) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_set<entity_type> set1;

    entity_type entity1 { 42, 5 };

    ASSERT_FALSE(set1.contains(entity1));
    set1.emplace_back(entity1);

    // Copy ctor - set2 is independent.
    entity_set<entity_type> set2 { set1 };

    ASSERT_EQ(set1.size(), 1);
    ASSERT_TRUE(set1.contains(entity1));
    ASSERT_EQ(set1[0], entity1);

    ASSERT_EQ(set2.size(), 1);
    ASSERT_TRUE(set2.contains(entity1));
    ASSERT_EQ(set2[0], entity1);

    // Mutate independently.
    ASSERT_FALSE(set1.contains(entity_type { 100, 5 }));
    set1.emplace_back(entity_type { 100, 5 });

    ASSERT_FALSE(set1.contains(entity_type { 200, 5 }));
    set1.emplace_back(entity_type { 200, 5 });

    ASSERT_FALSE(set2.contains(entity_type { 300, 5 }));
    set2.emplace_back(entity_type { 300, 5 });

    // Copy assignment - set2 replaced by set1's contents.
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

// ============================================================================
// Move - move ctor and move assignment; source left empty
// ============================================================================
TEST(EntitySet, Move) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_set<entity_type> set1;

    entity_type entity1 { 42, 5 };

    ASSERT_FALSE(set1.contains(entity1));
    set1.emplace_back(entity1);

    // Move ctor.
    entity_set<entity_type> set2 { std::move(set1) };

    ASSERT_EQ(set1.size(), 0);
    ASSERT_FALSE(set1.contains(entity1));

    ASSERT_EQ(set2.size(), 1);
    ASSERT_TRUE(set2.contains(entity1));
    ASSERT_EQ(set2[0], entity1);

    // Moved-from set can be reused.
    ASSERT_FALSE(set1.contains(entity_type { 100, 5 }));
    set1.emplace_back(entity_type { 100, 5 });

    ASSERT_FALSE(set1.contains(entity_type { 200, 5 }));
    set1.emplace_back(entity_type { 200, 5 });

    ASSERT_FALSE(set2.contains(entity_type { 300, 5 }));
    set2.emplace_back(entity_type { 300, 5 });

    // Move assignment.
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

// ============================================================================
// Emplace - insert entities; verify version-checked lookup and stale rejection
// ============================================================================
TEST(EntitySet, Emplace) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_set<entity_type> set;

    entity_type entity1 { 42, 5 };

    ASSERT_FALSE(set.occupied(entity1.id()));
    ASSERT_FALSE(set.contains(entity1));
    ASSERT_EQ(set.checked_index(entity1), entity_set<entity_type>::null_entity_index);
    size_t index1 = set.emplace_back(entity1);

    ASSERT_EQ(set.size(), 1);
    ASSERT_TRUE(set.occupied(entity1.id()));
    ASSERT_TRUE(set.contains(entity1));
    ASSERT_EQ(index1, set.index(entity1));
    ASSERT_EQ(index1, set.checked_index(entity1));
    ASSERT_EQ(set[index1], entity1);

    {
        // Stale entity: same id, older version - occupied() true, contains() false.
        entity_type stale { 42, 3 };

        ASSERT_TRUE(set.occupied(stale.id()));
        ASSERT_FALSE(set.contains(stale));
        ASSERT_EQ(set.index(stale), set.index(entity1));                // unsafe index: id match
        ASSERT_EQ(set.checked_index(stale), entity_set<entity_type>::null_entity_index);  // safe: version mismatch
    }

    entity_type entity2 { 100, 10 };

    ASSERT_FALSE(set.occupied(entity2.id()));
    ASSERT_FALSE(set.contains(entity2));
    ASSERT_EQ(set.checked_index(entity2), entity_set<entity_type>::null_entity_index);
    size_t index2 = set.emplace_back(entity2);

    ASSERT_EQ(set.size(), 2);
    ASSERT_TRUE(set.occupied(entity2.id()));
    ASSERT_TRUE(set.contains(entity2));
    ASSERT_EQ(index2, set.index(entity2));
    ASSERT_EQ(index2, set.checked_index(entity2));
    ASSERT_EQ(set[index2], entity2);

    entity_type entity3 { 200, 15 };

    ASSERT_FALSE(set.occupied(entity3.id()));
    ASSERT_FALSE(set.contains(entity3));
    ASSERT_EQ(set.checked_index(entity3), entity_set<entity_type>::null_entity_index);
    size_t index3 = set.emplace_back(entity3);

    ASSERT_EQ(set.size(), 3);
    ASSERT_TRUE(set.occupied(entity3.id()));
    ASSERT_TRUE(set.contains(entity3));
    ASSERT_EQ(index3, set.index(entity3));
    ASSERT_EQ(index3, set.checked_index(entity3));
    ASSERT_EQ(set[index3], entity3);

    entity_type entity4 { 300, 20 };

    ASSERT_FALSE(set.occupied(entity4.id()));
    ASSERT_FALSE(set.contains(entity4));
    ASSERT_EQ(set.checked_index(entity4), entity_set<entity_type>::null_entity_index);
    size_t index4 = set.emplace_back(entity4);

    ASSERT_EQ(set.size(), 4);
    ASSERT_TRUE(set.occupied(entity4.id()));
    ASSERT_TRUE(set.contains(entity4));
    ASSERT_EQ(index4, set.index(entity4));
    ASSERT_EQ(index4, set.checked_index(entity4));
    ASSERT_EQ(set[index4], entity4);
}

// ============================================================================
// Erase - remove entities; back-fill compacts, re-emplace with newer version
// ============================================================================
TEST(EntitySet, Erase) {
    using entity_type = basic_entity<uint32_t, uint16_t>;

    entity_set<entity_type> set;

    entity_type entity1 { 42, 5 };
    entity_type entity2 { 100, 10 };
    entity_type entity3 { 200, 15 };
    entity_type entity4 { 300, 20 };

    ASSERT_FALSE(set.occupied(entity1.id()));
    ASSERT_FALSE(set.contains(entity1));
    ASSERT_EQ(set.checked_index(entity1), entity_set<entity_type>::null_entity_index);
    set.emplace_back(entity1);

    ASSERT_FALSE(set.occupied(entity2.id()));
    ASSERT_FALSE(set.contains(entity2));
    ASSERT_EQ(set.checked_index(entity2), entity_set<entity_type>::null_entity_index);
    set.emplace_back(entity2);

    ASSERT_FALSE(set.occupied(entity3.id()));
    ASSERT_FALSE(set.contains(entity3));
    ASSERT_EQ(set.checked_index(entity3), entity_set<entity_type>::null_entity_index);
    set.emplace_back(entity3);

    ASSERT_FALSE(set.occupied(entity4.id()));
    ASSERT_FALSE(set.contains(entity4));
    ASSERT_EQ(set.checked_index(entity4), entity_set<entity_type>::null_entity_index);
    set.emplace_back(entity4);

    ASSERT_EQ(set.size(), 4);
    ASSERT_TRUE(set.occupied(entity1.id()));
    ASSERT_TRUE(set.occupied(entity2.id()));
    ASSERT_TRUE(set.occupied(entity3.id()));
    ASSERT_TRUE(set.occupied(entity4.id()));
    ASSERT_TRUE(set.contains(entity1));
    ASSERT_TRUE(set.contains(entity2));
    ASSERT_TRUE(set.contains(entity3));
    ASSERT_TRUE(set.contains(entity4));
    ASSERT_EQ(set.index(entity1), 0);
    ASSERT_EQ(set.index(entity2), 1);
    ASSERT_EQ(set.index(entity3), 2);
    ASSERT_EQ(set.index(entity4), 3);
    ASSERT_EQ(set.checked_index(entity1), 0);
    ASSERT_EQ(set.checked_index(entity2), 1);
    ASSERT_EQ(set.checked_index(entity3), 2);
    ASSERT_EQ(set.checked_index(entity4), 3);

    // Erase entity2 - back-filled, compacted.
    set.erase(entity2);

    ASSERT_EQ(set.size(), 3);
    ASSERT_TRUE(set.occupied(entity1.id()));
    ASSERT_FALSE(set.occupied(entity2.id()));   // id slot cleared
    ASSERT_TRUE(set.occupied(entity3.id()));
    ASSERT_TRUE(set.occupied(entity4.id()));
    ASSERT_TRUE(set.contains(entity1));
    ASSERT_FALSE(set.contains(entity2));
    ASSERT_TRUE(set.contains(entity3));
    ASSERT_TRUE(set.contains(entity4));
    ASSERT_EQ(set.checked_index(entity2), entity_set<entity_type>::null_entity_index);
    ASSERT_EQ(set.index(entity1), 0);
    ASSERT_EQ(set.index(entity4), 1);           // entity4 moved up
    ASSERT_EQ(set.index(entity3), 2);

    set.erase(entity1);

    ASSERT_EQ(set.size(), 2);
    ASSERT_FALSE(set.occupied(entity1.id()));
    ASSERT_FALSE(set.occupied(entity2.id()));
    ASSERT_TRUE(set.occupied(entity3.id()));
    ASSERT_TRUE(set.occupied(entity4.id()));
    ASSERT_FALSE(set.contains(entity1));
    ASSERT_FALSE(set.contains(entity2));
    ASSERT_TRUE(set.contains(entity3));
    ASSERT_TRUE(set.contains(entity4));
    ASSERT_EQ(set.checked_index(entity2), entity_set<entity_type>::null_entity_index);
    ASSERT_EQ(set.checked_index(entity1), entity_set<entity_type>::null_entity_index);
    ASSERT_EQ(set.index(entity3), 0);           // entity3 moved up
    ASSERT_EQ(set.index(entity4), 1);

    set.erase(entity4);

    ASSERT_EQ(set.size(), 1);
    ASSERT_FALSE(set.occupied(entity1.id()));
    ASSERT_FALSE(set.occupied(entity2.id()));
    ASSERT_TRUE(set.occupied(entity3.id()));
    ASSERT_FALSE(set.occupied(entity4.id()));
    ASSERT_FALSE(set.contains(entity1));
    ASSERT_FALSE(set.contains(entity2));
    ASSERT_TRUE(set.contains(entity3));
    ASSERT_FALSE(set.contains(entity4));
    ASSERT_EQ(set.checked_index(entity2), entity_set<entity_type>::null_entity_index);
    ASSERT_EQ(set.checked_index(entity1), entity_set<entity_type>::null_entity_index);
    ASSERT_EQ(set.checked_index(entity4), entity_set<entity_type>::null_entity_index);
    ASSERT_EQ(set.index(entity3), 0);

    // Erase last entity - set becomes empty.
    set.erase(entity3);

    ASSERT_TRUE(set.empty());
    ASSERT_FALSE(set.occupied(entity1.id()));
    ASSERT_FALSE(set.occupied(entity2.id()));
    ASSERT_FALSE(set.occupied(entity3.id()));
    ASSERT_FALSE(set.occupied(entity4.id()));
    ASSERT_FALSE(set.contains(entity1));
    ASSERT_FALSE(set.contains(entity2));
    ASSERT_FALSE(set.contains(entity3));
    ASSERT_FALSE(set.contains(entity4));
    ASSERT_EQ(set.checked_index(entity2), entity_set<entity_type>::null_entity_index);
    ASSERT_EQ(set.checked_index(entity1), entity_set<entity_type>::null_entity_index);
    ASSERT_EQ(set.checked_index(entity4), entity_set<entity_type>::null_entity_index);
    ASSERT_EQ(set.checked_index(entity3), entity_set<entity_type>::null_entity_index);

    // Re-emplace same id with a newer version - erased id can be reused with a fresh entity.
    entity_type e1_new { 42, 10 };

    ASSERT_FALSE(set.occupied(e1_new.id()));   // id was freed by erase
    ASSERT_FALSE(set.contains(e1_new));
    ASSERT_EQ(set.checked_index(e1_new), entity_set<entity_type>::null_entity_index);

    set.emplace_back(e1_new);

    ASSERT_EQ(set.size(), 1);
    ASSERT_TRUE(set.occupied(e1_new.id()));
    ASSERT_TRUE(set.contains(e1_new));
    ASSERT_EQ(set.index(e1_new), 0);
    ASSERT_EQ(set.checked_index(e1_new), 0);

    // Stale old-version entity still rejected.
    ASSERT_EQ(set.index(entity1), set.index(e1_new));                      // same id -> same unsafe index
    ASSERT_EQ(set.checked_index(entity1), entity_set<entity_type>::null_entity_index);  // old version rejected
    ASSERT_EQ(set.checked_index(e1_new), set.index(e1_new));               // new version accepted
}
