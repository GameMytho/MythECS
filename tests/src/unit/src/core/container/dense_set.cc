#include <gtest/gtest.h>
#include <core/container/dense_set.hpp>

using namespace myth::core::container;

using dense_set_type = dense_set<int>;

// Helper hasher templates for collision tests.
// (dense_set's Hash parameter is template<typename> - must be a class template, not a plain class.)
template<typename>
struct all_zero_hash {
    size_t operator()(int) const noexcept { return 0; }
};

template<typename>
struct lowbits_hash {
    size_t operator()(int k) const noexcept { return static_cast<size_t>(k) & 0x3; }
};

// ============================================================================
// Functionalities - empty, emplace, contains, index, operator[], clear
// ============================================================================
TEST(DenseSet, Functionalities) {
    dense_set_type set;

    // A default-constructed set is empty.
    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0);
    ASSERT_EQ(set.capacity(), 0);

    // Load factor and bucket properties on empty set.
    ASSERT_FLOAT_EQ(set.load_factor(), 0.0f);
    ASSERT_GE(set.bucket_count(), dense_set_type::minimum_bucket_count);
    ASSERT_EQ(set.bucket_count() & (set.bucket_count() - 1), 0u);  // power of two
    ASSERT_FLOAT_EQ(set.max_load_factor(), dense_set_type::default_threshold);

    // Missing key: contains() is false, index() returns null_key_index.
    ASSERT_FALSE(set.contains(13));
    ASSERT_EQ(set.index(13), dense_set_type::null_key_index);

    // Emplace returns true for the newly inserted key.
    bool res13 = set.emplace(13);
    ASSERT_EQ(res13, true);
    ASSERT_EQ(set[0], 13);

    // After emplace, the set is non-empty and has allocated capacity.
    ASSERT_FALSE(set.empty());
    ASSERT_EQ(set.size(), 1);
    ASSERT_NE(set.capacity(), 0);

    // The emplaced key is reachable via contains / index / operator[].
    ASSERT_TRUE(set.contains(13));
    ASSERT_EQ(set.index(13), 0);
    ASSERT_EQ(set[0], 13);

    // Duplicate emplace - returns false for the existing element.
    ASSERT_EQ(set.emplace(13), false);
    ASSERT_EQ(set.size(), 1);
    ASSERT_TRUE(set.contains(13));
    ASSERT_EQ(set[set.index(13)], 13);

    // Load factor after one insert.
    ASSERT_FLOAT_EQ(set.load_factor(), 1.0f / static_cast<float>(set.bucket_count()));

    // After clear, the set is back to the empty state.
    set.clear();

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0);
    ASSERT_GE(set.capacity(), 0);
    ASSERT_FALSE(set.contains(13));
    ASSERT_EQ(set.index(13), dense_set_type::null_key_index);
    ASSERT_FLOAT_EQ(set.load_factor(), 0.0f);
}

// ============================================================================
// Constructors - default, (capacity, allocator), (+hasher), (+keyeq)
// ============================================================================
TEST(DenseSet, Constructors) {
    // Default constructor.
    dense_set_type set1;

    ASSERT_TRUE(set1.empty());
    ASSERT_EQ(set1.size(), 0);
    ASSERT_GE(set1.capacity(), 0);
    ASSERT_GE(set1.bucket_count(), dense_set_type::minimum_bucket_count);
    ASSERT_EQ(set1.bucket_count() & (set1.bucket_count() - 1), 0u);  // power of two

    // (capacity, allocator).
    dense_set_type set2{ 10, std::allocator<int>{} };

    ASSERT_TRUE(set2.empty());
    ASSERT_EQ(set2.size(), 0);
    ASSERT_GE(set2.capacity(), 10);
    ASSERT_EQ(set2.bucket_count(), 16);

    // (capacity, hasher, allocator).
    dense_set_type set3{ 10, std::hash<int>{}, std::allocator<int>{} };

    ASSERT_TRUE(set3.empty());
    ASSERT_EQ(set3.size(), 0);
    ASSERT_GE(set3.capacity(), 10);
    ASSERT_EQ(set3.bucket_count(), 16);

    // (capacity, hasher, keyeq, allocator).
    dense_set_type set4{ 10, std::hash<int>{}, std::equal_to<int>{}, std::allocator<int>{} };

    ASSERT_TRUE(set4.empty());
    ASSERT_EQ(set4.size(), 0);
    ASSERT_GE(set4.capacity(), 10);
    ASSERT_EQ(set4.bucket_count(), 16);
}

// ============================================================================
// Copy - copy ctor, copy assignment, allocator-extended copy ctor
// ============================================================================
TEST(DenseSet, Copy) {
    dense_set_type set1;

    ASSERT_FALSE(set1.contains(13));
    ASSERT_EQ(set1.index(13), dense_set_type::null_key_index);
    bool res13 = set1.emplace(13);
    ASSERT_EQ(res13, true);

    // Copy construct - set2 is an independent copy of set1.
    dense_set_type set2{ set1 };

    ASSERT_EQ(set1.size(), 1);
    ASSERT_TRUE(set1.contains(13));
    ASSERT_NE(set1.index(13), dense_set_type::null_key_index);
    ASSERT_EQ(set1[set1.index(13)], 13);

    ASSERT_EQ(set2.size(), 1);
    ASSERT_TRUE(set2.contains(13));
    ASSERT_NE(set2.index(13), dense_set_type::null_key_index);
    ASSERT_EQ(set2[set2.index(13)], 13);

    // Mutate the two sets independently.
    ASSERT_FALSE(set1.contains(42));
    ASSERT_EQ(set1.index(42), dense_set_type::null_key_index);
    bool res42 = set1.emplace(42);
    ASSERT_EQ(res42, true);

    ASSERT_FALSE(set1.contains(100));
    ASSERT_EQ(set1.index(100), dense_set_type::null_key_index);
    bool res100 = set1.emplace(100);
    ASSERT_EQ(res100, true);

    ASSERT_FALSE(set2.contains(0));
    ASSERT_EQ(set2.index(0), dense_set_type::null_key_index);
    bool res0 = set2.emplace(0);
    ASSERT_EQ(res0, true);

    // Copy assignment - set2 is replaced by a copy of set1.
    set2 = set1;

    ASSERT_EQ(set1.size(), 3);
    ASSERT_TRUE(set1.contains(13));
    ASSERT_TRUE(set1.contains(42));
    ASSERT_TRUE(set1.contains(100));

    ASSERT_EQ(set2.size(), 3);
    ASSERT_TRUE(set2.contains(13));
    ASSERT_TRUE(set2.contains(42));
    ASSERT_TRUE(set2.contains(100));

    // Allocator - extended copy ctor.
    dense_set_type set3{ set1, std::allocator<int>{} };

    ASSERT_EQ(set3.size(), 3);
    ASSERT_TRUE(set3.contains(13));
    ASSERT_TRUE(set3.contains(42));
    ASSERT_TRUE(set3.contains(100));
    ASSERT_EQ(set1.size(), 3);  // source unchanged

    // Self copy-assignment - must be safe and leave contents intact.
    set1 = set1;

    ASSERT_EQ(set1.size(), 3);
    ASSERT_TRUE(set1.contains(13));
    ASSERT_TRUE(set1.contains(42));
    ASSERT_TRUE(set1.contains(100));
}

// ============================================================================
// Move - move ctor, move assignment, allocator-extended move ctor
// ============================================================================
TEST(DenseSet, Move) {
    dense_set_type set1;

    ASSERT_FALSE(set1.contains(13));
    ASSERT_EQ(set1.index(13), dense_set_type::null_key_index);
    bool res13 = set1.emplace(13);
    ASSERT_EQ(res13, true);

    // Move construct - set1's resources transfer to set2; set1 becomes empty.
    dense_set_type set2{ std::move(set1) };

    ASSERT_TRUE(set1.empty());
    ASSERT_EQ(set1.size(), 0);
    ASSERT_FALSE(set1.contains(13));
    ASSERT_EQ(set1.index(13), dense_set_type::null_key_index);

    ASSERT_EQ(set2.size(), 1);
    ASSERT_TRUE(set2.contains(13));
    ASSERT_NE(set2.index(13), dense_set_type::null_key_index);
    ASSERT_EQ(set2[set2.index(13)], 13);

    // A moved-from set is valid-but-unspecified; emplace() re-initializes it automatically.
    ASSERT_FALSE(set1.contains(42));
    ASSERT_EQ(set1.index(42), dense_set_type::null_key_index);
    bool res42 = set1.emplace(42);
    ASSERT_EQ(res42, true);

    ASSERT_FALSE(set1.contains(100));
    ASSERT_EQ(set1.index(100), dense_set_type::null_key_index);
    bool res100 = set1.emplace(100);
    ASSERT_EQ(res100, true);

    ASSERT_FALSE(set2.contains(0));
    ASSERT_EQ(set2.index(0), dense_set_type::null_key_index);
    bool res0 = set2.emplace(0);
    ASSERT_EQ(res0, true);

    // Move assignment - set1's resources transfer to set2; set2's old content is freed.
    set2 = std::move(set1);

    ASSERT_TRUE(set1.empty());
    ASSERT_EQ(set1.size(), 0);

    ASSERT_EQ(set2.size(), 2);
    ASSERT_TRUE(set2.contains(42));
    ASSERT_TRUE(set2.contains(100));

    // Allocator-extended move ctor.
    dense_set_type set3;

    ASSERT_FALSE(set3.contains(13));
    bool res13_new = set3.emplace(13);
    ASSERT_EQ(res13, true);
    ASSERT_FALSE(set3.contains(42));
    bool res42_new = set3.emplace(42);
    ASSERT_EQ(res42, true);

    dense_set_type set4{ std::move(set3), std::allocator<int>{} };

    ASSERT_EQ(set4.size(), 2);
    ASSERT_TRUE(set4.contains(13));
    ASSERT_TRUE(set4.contains(42));

    // Self move-assignment - must leave the set in a usable state.
    set4 = std::move(set4);

    // After self-move-assignment, the set must still be usable - clear and re-populate.
    set4.clear();
    ASSERT_TRUE(set4.empty());

    bool res7 = set4.emplace(7);
    ASSERT_EQ(res7, true);
    ASSERT_TRUE(set4.contains(7));
    ASSERT_EQ(set4.size(), 1);
}

// ============================================================================
// Emplace - insert elements sequentially, verifying return value and lookups
// ============================================================================
TEST(DenseSet, Emplace) {
    dense_set_type set;

    // Insert 13 - return true for new key
    ASSERT_FALSE(set.contains(13));
    ASSERT_EQ(set.index(13), dense_set_type::null_key_index);

    bool res13 = set.emplace(13);
    ASSERT_EQ(res13, true);
    ASSERT_EQ(set[0], 13);
    ASSERT_EQ(set.index(13), 0);
    ASSERT_EQ(set.size(), 1);
    ASSERT_TRUE(set.contains(13));

    // Insert 42.
    ASSERT_FALSE(set.contains(42));
    ASSERT_EQ(set.index(42), dense_set_type::null_key_index);

    bool res42 = set.emplace(42);
    ASSERT_EQ(res42, true);
    ASSERT_EQ(set[1], 42);
    ASSERT_EQ(set.index(42), 1);
    ASSERT_EQ(set.size(), 2);
    ASSERT_TRUE(set.contains(42));

    // Insert 100.
    ASSERT_FALSE(set.contains(100));
    ASSERT_EQ(set.index(100), dense_set_type::null_key_index);

    bool res100 = set.emplace(100);
    ASSERT_EQ(res100, true);
    ASSERT_EQ(set[2], 100);
    ASSERT_EQ(set.index(100), 2);
    ASSERT_EQ(set.size(), 3);
    ASSERT_TRUE(set.contains(100));

    // Insert 0.
    ASSERT_FALSE(set.contains(0));
    ASSERT_EQ(set.index(0), dense_set_type::null_key_index);

    bool res0 = set.emplace(0);
    ASSERT_EQ(res0, true);
    ASSERT_EQ(set[3], 0);
    ASSERT_EQ(set.index(0), 3);
    ASSERT_EQ(set.size(), 4);
    ASSERT_TRUE(set.contains(0));

    // Duplicate emplace - return false for the existing element.
    ASSERT_EQ(set.emplace(0), false);
    ASSERT_EQ(set.size(), 4);
    ASSERT_TRUE(set.contains(0));

    ASSERT_EQ(set.emplace(100), false);
    ASSERT_EQ(set.size(), 4);
    ASSERT_TRUE(set.contains(100));

    // Emplace after clear - return value refers to new position.
    set.clear();
    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0);

    bool res42_new = set.emplace(42);
    ASSERT_EQ(res42_new, true);
    ASSERT_EQ(set[0], 42);
    ASSERT_EQ(set.index(42), 0);
    ASSERT_EQ(set.size(), 1);
    ASSERT_TRUE(set.contains(42));
}

// ============================================================================
// Erase - remove elements one by one; covers both move_and_pop paths:
//   (a) index != last_idx  -> back-fill then pop
//   (b) index == last_idx  -> direct pop
// ============================================================================
TEST(DenseSet, Erase) {
    dense_set_type set;

    ASSERT_FALSE(set.contains(13));
    ASSERT_EQ(set.index(13), dense_set_type::null_key_index);
    bool res13 = set.emplace(13);
    ASSERT_EQ(res13, true);

    ASSERT_FALSE(set.contains(42));
    ASSERT_EQ(set.index(42), dense_set_type::null_key_index);
    bool res42 = set.emplace(42);
    ASSERT_EQ(res42, true);

    ASSERT_FALSE(set.contains(100));
    ASSERT_EQ(set.index(100), dense_set_type::null_key_index);
    bool res100 = set.emplace(100);
    ASSERT_EQ(res100, true);

    ASSERT_FALSE(set.contains(0));
    ASSERT_EQ(set.index(0), dense_set_type::null_key_index);
    bool res0 = set.emplace(0);
    ASSERT_EQ(res0, true);

    ASSERT_EQ(set.size(), 4);
    ASSERT_TRUE(set.contains(13));
    ASSERT_TRUE(set.contains(42));
    ASSERT_TRUE(set.contains(100));
    ASSERT_TRUE(set.contains(0));

    // Erase 42 (not the last element -> back-fill path).
    set.erase(42);

    ASSERT_EQ(set.size(), 3);
    ASSERT_FALSE(set.contains(42));
    ASSERT_EQ(set.index(42), dense_set_type::null_key_index);
    ASSERT_TRUE(set.contains(13));
    ASSERT_TRUE(set.contains(100));
    ASSERT_TRUE(set.contains(0));

    // Erase 13 (not the last element -> back-fill path).
    set.erase(13);

    ASSERT_EQ(set.size(), 2);
    ASSERT_FALSE(set.contains(42));
    ASSERT_FALSE(set.contains(13));
    ASSERT_EQ(set.index(13), dense_set_type::null_key_index);
    ASSERT_TRUE(set.contains(100));
    ASSERT_TRUE(set.contains(0));

    // Erase 0 (now the last element -> direct pop path).
    set.erase(0);

    ASSERT_EQ(set.size(), 1);
    ASSERT_FALSE(set.contains(42));
    ASSERT_FALSE(set.contains(13));
    ASSERT_FALSE(set.contains(0));
    ASSERT_EQ(set.index(0), dense_set_type::null_key_index);
    ASSERT_TRUE(set.contains(100));

    // Erase 100 (the sole remaining element -> direct pop path).
    set.erase(100);

    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0);
    ASSERT_FALSE(set.contains(42));
    ASSERT_FALSE(set.contains(13));
    ASSERT_FALSE(set.contains(100));
    ASSERT_FALSE(set.contains(0));
    ASSERT_EQ(set.index(100), dense_set_type::null_key_index);

    // Erase from empty set - must be a safe no-op.
    set.erase(999);
    ASSERT_TRUE(set.empty());
    ASSERT_EQ(set.size(), 0);

    // Erase non-existent key from non-empty set - must be a no-op.
    bool res13_new = set.emplace(13);
    ASSERT_EQ(res13_new, true);
    bool res42_new = set.emplace(42);
    ASSERT_EQ(res42_new, true);
    ASSERT_EQ(set.size(), 2);

    set.erase(999);
    ASSERT_EQ(set.size(), 2);
    ASSERT_TRUE(set.contains(13));
    ASSERT_TRUE(set.contains(42));

    // Emplace after erase - chain integrity preserved.
    set.erase(13);
    ASSERT_FALSE(set.contains(13));
    ASSERT_EQ(set.emplace(13), true);
    ASSERT_TRUE(set.contains(13));
    ASSERT_EQ(set.size(), 2);
    ASSERT_TRUE(set.contains(42));
}

// ============================================================================
// Rehash - insert enough elements to trigger rehash and verify correctness;
//          also covers max_load_factor, reserve, shrink_to_fit, and collisions
// ============================================================================
TEST(DenseSet, Rehash) {
    dense_set_type set;

    // default: minimum_bucket_count=8, threshold=0.875 => rehash when size > 7
    // Insert 9 elements to trigger at least one rehash (8 -> 16 buckets).
    for (int i = 0; i < 9; ++i) {
        ASSERT_FALSE(set.contains(i));
        bool res = set.emplace(i);
	ASSERT_EQ(res, true);
        ASSERT_TRUE(set.contains(i));
        ASSERT_EQ(set[set.index(i)], i);

        // bucket_count must always be a power of two >= minimum_bucket_count.
        ASSERT_GE(set.bucket_count(), dense_set_type::minimum_bucket_count);
        ASSERT_EQ(set.bucket_count() & (set.bucket_count() - 1), 0u);
    }

    ASSERT_EQ(set.size(), 9);

    // All elements remain reachable after rehash.
    for (int i = 0; i < 9; ++i) {
        ASSERT_TRUE(set.contains(i));
        ASSERT_EQ(set[set.index(i)], i);
    }

    // --- max_load_factor ---
    // Lower the threshold - should trigger bucket growth.
    size_t buckets_before = set.bucket_count();
    set.max_load_factor(0.5f);
    ASSERT_GT(set.bucket_count(), buckets_before);
    ASSERT_FLOAT_EQ(set.max_load_factor(), 0.5f);

    for (int i = 0; i < 9; ++i) {
        ASSERT_TRUE(set.contains(i));
    }

    // Raise the threshold - buckets should shrink.
    set.max_load_factor(0.875f);
    ASSERT_FLOAT_EQ(set.max_load_factor(), 0.875f);
    ASSERT_LE(set.bucket_count(), buckets_before);

    for (int i = 0; i < 9; ++i) {
        ASSERT_TRUE(set.contains(i));
    }

    // --- reserve ---
    dense_set_type set2;
    set2.reserve(128);
    ASSERT_GE(set2.capacity(), 128);

    size_t cap_before = set2.capacity();
    for (int i = 0; i < 100; ++i) {
        bool res = set2.emplace(i);
	ASSERT_EQ(res, true);
    }
    ASSERT_EQ(set2.capacity(), cap_before);  // no density reallocation
    for (int i = 0; i < 100; ++i) {
        ASSERT_TRUE(set2.contains(i));
    }

    // --- shrink_to_fit ---
    set2.clear();
    ASSERT_TRUE(set2.empty());
    set2.shrink_to_fit();

    ASSERT_GE(set2.bucket_count(), dense_set_type::minimum_bucket_count);
    ASSERT_EQ(set2.bucket_count() & (set2.bucket_count() - 1), 0u);

    // Usable after shrink.
    bool res42 = set2.emplace(42);
    ASSERT_EQ(res42, true);
    ASSERT_TRUE(set2.contains(42));

    // --- hash collision stress ---
    // Use a hasher that maps all keys to the same bucket.
    using collision_set_type = dense_set<int, all_zero_hash>;
    collision_set_type collision_set;

    for (int i = 0; i < 50; ++i) {
        ASSERT_FALSE(collision_set.contains(i));
        bool res = collision_set.emplace(i);
	ASSERT_EQ(res, true);
        ASSERT_TRUE(collision_set.contains(i));
    }
    ASSERT_EQ(collision_set.size(), 50);

    // Erase from collision chain.
    collision_set.erase(0);
    collision_set.erase(25);
    collision_set.erase(49);
    ASSERT_EQ(collision_set.size(), 47);

    for (int i = 0; i < 50; ++i) {
        if (i == 0 || i == 25 || i == 49) {
            ASSERT_FALSE(collision_set.contains(i));
        } else {
            ASSERT_TRUE(collision_set.contains(i)) << "i = " << i;
        }
    }

    // Re-insert erased keys.
    bool res0 = collision_set.emplace(0);
    ASSERT_EQ(res0, true);
    bool res25 = collision_set.emplace(25);
    ASSERT_EQ(res25, true);
    bool res49 = collision_set.emplace(49);
    ASSERT_EQ(res49, true);
    ASSERT_EQ(collision_set.size(), 50);
    ASSERT_TRUE(collision_set.contains(0));
    ASSERT_TRUE(collision_set.contains(25));
    ASSERT_TRUE(collision_set.contains(49));
}

// ============================================================================
// Swap - swap two elements by density index; verifies chain-patching
// ============================================================================
TEST(DenseSet, Swap) {
    dense_set_type set;

    ASSERT_FALSE(set.contains(13));
    ASSERT_EQ(set.index(13), dense_set_type::null_key_index);
    bool res13 = set.emplace(13);
    ASSERT_EQ(res13, true);

    ASSERT_FALSE(set.contains(42));
    ASSERT_EQ(set.index(42), dense_set_type::null_key_index);
    bool res42 = set.emplace(42);
    ASSERT_EQ(res42, true);

    ASSERT_FALSE(set.contains(100));
    ASSERT_EQ(set.index(100), dense_set_type::null_key_index);
    bool res100 = set.emplace(100);
    ASSERT_EQ(res100, true);

    ASSERT_EQ(set.size(), 3);

    // Record initial indices.
    size_t idx13 = set.index(13);
    size_t idx42 = set.index(42);
    size_t idx100 = set.index(100);

    ASSERT_EQ(set[idx13], 13);
    ASSERT_EQ(set[idx42], 42);
    ASSERT_EQ(set[idx100], 100);

    // Swap 13 and 100 - indices stay, values exchange; index() table updated.
    set.swap(idx13, idx100);

    ASSERT_EQ(set.size(), 3);
    ASSERT_EQ(set[idx13], 100);
    ASSERT_EQ(set[idx42], 42);
    ASSERT_EQ(set[idx100], 13);
    ASSERT_EQ(set.index(100), idx13);
    ASSERT_EQ(set.index(42), idx42);
    ASSERT_EQ(set.index(13), idx100);

    // Swap using values looked up via index().
    set.swap(set.index(100), set.index(42));

    ASSERT_EQ(set[idx13], 42);
    ASSERT_EQ(set[idx42], 100);
    ASSERT_EQ(set[idx100], 13);
    ASSERT_EQ(set.index(42), idx13);
    ASSERT_EQ(set.index(100), idx42);
    ASSERT_EQ(set.index(13), idx100);

    // Self-swap - must be a no-op.
    set.swap(set.index(42), set.index(42));

    ASSERT_EQ(set.size(), 3);
    ASSERT_EQ(set.index(42), idx13);
    ASSERT_EQ(set[idx13], 42);

    // Swap under hash collisions - keys in the same bucket chain.
    using cset_type = dense_set<int, lowbits_hash>;
    cset_type cset;

    // Insert enough elements to force chain collisions.
    for (int i = 0; i < 20; ++i) {
        bool res = cset.emplace(i);
	ASSERT_EQ(res, true);
    }

    size_t a = cset.index(3);
    size_t b = cset.index(7);  // 3 & 0x3 = 3, 7 & 0x3 = 3 - same bucket
    ASSERT_NE(a, b);
    ASSERT_NE(a, cset_type::null_key_index);
    ASSERT_NE(b, cset_type::null_key_index);

    cset.swap(a, b);

    // After swap, keys moved to each other's indices.
    ASSERT_EQ(cset[a], 7);
    ASSERT_EQ(cset[b], 3);
    ASSERT_EQ(cset.index(3), b);
    ASSERT_EQ(cset.index(7), a);

    // All elements still reachable.
    for (int i = 0; i < 20; ++i) {
        ASSERT_TRUE(cset.contains(i)) << "i = " << i;
    }
}
