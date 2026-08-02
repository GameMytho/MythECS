#include <cstdint>
#include <span>
#include <utility>

#include <gtest/gtest.h>
#include <ecs/entity.hpp>
#include <core/type_info.hpp>
#include <storage/archtype.hpp>

using namespace myth::ecs;
using namespace myth::storage;

namespace {
    // Component types. Each type doubles as a component identity: its id comes from
    // type_info_generator::id<T>() and its layout from info<T>(), exactly as real
    // ECS code registers components. Tag is an empty type, excluded by the caller
    // before reaching the archtype constructor -- the archtype only deals with data
    // components (non-empty types).
    struct Position { float x; float y; };   // data component
    struct Velocity { float dx; float dy; }; // data component

    const ::myth::core::type_info& INFO_POS = ::myth::core::type_info_generator::info<Position>();
    const ::myth::core::type_info& INFO_VEL = ::myth::core::type_info_generator::info<Velocity>();
}

// ============================================================================
// Functionalities - construct, emplace, get_data, get_tick, erase lifecycle
// ============================================================================
TEST(Archtype, Functionalities) {
    using entity_type = basic_entity<uint32_t, uint16_t>;
    using archtype_type = archtype<entity_type>;

    // Two data components (Position, Velocity). The archtype only receives type_info
    // for data components -- tags are filtered out by the caller (archtype_storage).
    const ::myth::core::type_info* infos[] = { &INFO_POS, &INFO_VEL };
    archtype_type arch(infos);

    // Freshly constructed: no entities yet.
    ASSERT_TRUE(arch.empty());
    ASSERT_EQ(arch.size(), 0);
    ASSERT_EQ(arch.capacity(), 0);

    // Emplace an entity carrying Position + Velocity data.
    // Data must be in the same pool order as the constructor's infos.
    Position p { 1.0f, 2.0f };
    Velocity v { 3.0f, 4.0f };
    void* datas[] = { &p, &v };

    auto idx = arch.emplace_back(10u, std::span<void*>(datas));

    ASSERT_EQ(idx, 0);
    ASSERT_FALSE(arch.empty());
    ASSERT_EQ(arch.size(), 1);
    ASSERT_GE(arch.capacity(), 1);

    // Component data is copied into the pools and retrievable by (col_idx, row_idx).
    // col_idx 0 = Position (first in infos), col_idx 1 = Velocity (second).
    ASSERT_EQ(static_cast<Position*>(arch.get_data(0, 0))->x, 1.0f);
    ASSERT_EQ(static_cast<Position*>(arch.get_data(0, 0))->y, 2.0f);
    ASSERT_EQ(static_cast<Velocity*>(arch.get_data(1, 0))->dx, 3.0f);
    ASSERT_EQ(static_cast<Velocity*>(arch.get_data(1, 0))->dy, 4.0f);

    // Ticks are initialized to zero for every component of a new entity.
    ASSERT_EQ(arch.get_tick(0, 0), 0u);
    ASSERT_EQ(arch.get_tick(1, 0), 0u);

    // Erasing the only (last) entity empties the archetype and reports null.
    auto swapped = arch.erase(0);

    ASSERT_EQ(swapped, entity_type::null_entity_id);
    ASSERT_TRUE(arch.empty());
    ASSERT_EQ(arch.size(), 0);
}

// ============================================================================
// Constructors - data components only; each entry creates a pool + tick pool
// ============================================================================
TEST(Archtype, Constructors) {
    using entity_type = basic_entity<uint32_t, uint16_t>;
    using archtype_type = archtype<entity_type>;

    // --- Single data component ---
    {
        const ::myth::core::type_info* infos[] = { &INFO_POS };
        archtype_type arch(infos);

        ASSERT_TRUE(arch.empty());

        Position p { 5.0f, 6.0f };
        void* datas[] = { &p };

        ASSERT_EQ(arch.emplace_back(1u, std::span<void*>(datas)), 0);
        ASSERT_EQ(arch.size(), 1);
        ASSERT_EQ(static_cast<Position*>(arch.get_data(0, 0))->x, 5.0f);
    }

    // --- Two data components with distinct types ---
    {
        const ::myth::core::type_info* infos[] = { &INFO_POS, &INFO_VEL };
        archtype_type arch(infos);

        Position p { 10.0f, 11.0f };
        Velocity v { 12.0f, 13.0f };
        void* datas[] = { &p, &v };

        ASSERT_EQ(arch.emplace_back(1u, std::span<void*>(datas)), 0);
        ASSERT_EQ(static_cast<Position*>(arch.get_data(0, 0))->x, 10.0f);
        ASSERT_EQ(static_cast<Velocity*>(arch.get_data(1, 0))->dx, 12.0f);
    }

    // --- Same type_info repeated (two pools of the same component type) ---
    {
        const ::myth::core::type_info* infos[] = { &INFO_POS, &INFO_POS };
        archtype_type arch(infos);

        Position p0 { 1.0f, 2.0f };
        Position p1 { 3.0f, 4.0f };
        void* datas[] = { &p0, &p1 };

        ASSERT_EQ(arch.emplace_back(1u, std::span<void*>(datas)), 0);
        ASSERT_EQ(static_cast<Position*>(arch.get_data(0, 0))->x, 1.0f);
        ASSERT_EQ(static_cast<Position*>(arch.get_data(1, 0))->x, 3.0f);
    }
}

// ============================================================================
// Move - move ctor and move assignment transfer contents; source left empty
// ============================================================================
TEST(Archtype, Move) {
    using entity_type = basic_entity<uint32_t, uint16_t>;
    using archtype_type = archtype<entity_type>;

    const ::myth::core::type_info* infos[] = { &INFO_POS };
    archtype_type arch1(infos);

    Position p { 42.0f, 43.0f };
    void* datas[] = { &p };

    ASSERT_EQ(arch1.emplace_back(10u, std::span<void*>(datas)), 0);
    ASSERT_EQ(arch1.size(), 1);

    // Move ctor - contents transfer to arch2, arch1 is left empty.
    archtype_type arch2(std::move(arch1));

    ASSERT_TRUE(arch1.empty());
    ASSERT_EQ(arch1.size(), 0);

    ASSERT_EQ(arch2.size(), 1);
    ASSERT_EQ(static_cast<Position*>(arch2.get_data(0, 0))->x, 42.0f);
    ASSERT_EQ(static_cast<Position*>(arch2.get_data(0, 0))->y, 43.0f);
    ASSERT_EQ(arch2.get_tick(0, 0), 0u);

    // A second, independently populated archetype to receive a move assignment.
    archtype_type arch3(infos);

    Position q { 7.0f, 8.0f };
    void* datas3[] = { &q };

    ASSERT_EQ(arch3.emplace_back(11u, std::span<void*>(datas3)), 0);
    ASSERT_EQ(arch3.size(), 1);

    // Move assignment - arch3 takes over arch2's contents, arch2 is left empty.
    arch3 = std::move(arch2);

    ASSERT_TRUE(arch2.empty());
    ASSERT_EQ(arch2.size(), 0);

    ASSERT_EQ(arch3.size(), 1);
    ASSERT_EQ(static_cast<Position*>(arch3.get_data(0, 0))->x, 42.0f);
    ASSERT_EQ(static_cast<Position*>(arch3.get_data(0, 0))->y, 43.0f);
}

// ============================================================================
// Emplace - multiple entities packed contiguously with zero-initialized ticks
// ============================================================================
TEST(Archtype, Emplace) {
    using entity_type = basic_entity<uint32_t, uint16_t>;
    using archtype_type = archtype<entity_type>;

    const ::myth::core::type_info* infos[] = { &INFO_POS, &INFO_VEL };
    archtype_type arch(infos);

    Position p0 { 10.0f, 11.0f }, p1 { 20.0f, 21.0f }, p2 { 30.0f, 31.0f };
    Velocity v0 { 1.0f, 2.0f }, v1 { 3.0f, 4.0f }, v2 { 5.0f, 6.0f };

    void* d0[] = { &p0, &v0 };
    void* d1[] = { &p1, &v1 };
    void* d2[] = { &p2, &v2 };

    // Entities are appended and report their packed index in insertion order.
    ASSERT_EQ(arch.emplace_back(10u, std::span<void*>(d0)), 0);
    ASSERT_EQ(arch.emplace_back(11u, std::span<void*>(d1)), 1);
    ASSERT_EQ(arch.emplace_back(12u, std::span<void*>(d2)), 2);

    ASSERT_EQ(arch.size(), 3);

    // Every slot holds the data it was emplaced with, across both pools.
    ASSERT_EQ(static_cast<Position*>(arch.get_data(0, 0))->x, 10.0f);
    ASSERT_EQ(static_cast<Position*>(arch.get_data(0, 1))->x, 20.0f);
    ASSERT_EQ(static_cast<Position*>(arch.get_data(0, 2))->x, 30.0f);
    ASSERT_EQ(static_cast<Velocity*>(arch.get_data(1, 0))->dx, 1.0f);
    ASSERT_EQ(static_cast<Velocity*>(arch.get_data(1, 1))->dx, 3.0f);
    ASSERT_EQ(static_cast<Velocity*>(arch.get_data(1, 2))->dx, 5.0f);

    // Ticks start at zero for each emplaced entity/component.
    ASSERT_EQ(arch.get_tick(0, 0), 0u);
    ASSERT_EQ(arch.get_tick(0, 1), 0u);
    ASSERT_EQ(arch.get_tick(0, 2), 0u);
    ASSERT_EQ(arch.get_tick(1, 0), 0u);
    ASSERT_EQ(arch.get_tick(1, 1), 0u);
    ASSERT_EQ(arch.get_tick(1, 2), 0u);
}

// ============================================================================
// Erase - swap-and-pop compaction; returns the id of the swapped-in entity
// ============================================================================
TEST(Archtype, Erase) {
    using entity_type = basic_entity<uint32_t, uint16_t>;
    using archtype_type = archtype<entity_type>;

    const ::myth::core::type_info* infos[] = { &INFO_POS };
    archtype_type arch(infos);

    // Three entities (ids 10/11/12) with distinguishable positions.
    Position p0 { 10.0f, 0.0f }, p1 { 20.0f, 0.0f }, p2 { 30.0f, 0.0f };

    void* d0[] = { &p0 };
    void* d1[] = { &p1 };
    void* d2[] = { &p2 };

    ASSERT_EQ(arch.emplace_back(10u, std::span<void*>(d0)), 0);
    ASSERT_EQ(arch.emplace_back(11u, std::span<void*>(d1)), 1);
    ASSERT_EQ(arch.emplace_back(12u, std::span<void*>(d2)), 2);
    ASSERT_EQ(arch.size(), 3);

    // Erase the first slot: the last entity (12) is swapped into index 0, so its
    // id is returned to let the caller patch its mapping.
    auto s0 = arch.erase(0);

    ASSERT_EQ(s0, 12u);
    ASSERT_EQ(arch.size(), 2);
    ASSERT_EQ(static_cast<Position*>(arch.get_data(0, 0))->x, 30.0f);  // entity 12 moved here
    ASSERT_EQ(static_cast<Position*>(arch.get_data(0, 1))->x, 20.0f);  // entity 11 unchanged

    // Erase the last slot: nothing is moved, so null is returned.
    auto s1 = arch.erase(1);

    ASSERT_EQ(s1, entity_type::null_entity_id);
    ASSERT_EQ(arch.size(), 1);
    ASSERT_EQ(static_cast<Position*>(arch.get_data(0, 0))->x, 30.0f);

    // Erase the remaining (only, hence last) entity: archetype becomes empty.
    auto s2 = arch.erase(0);

    ASSERT_EQ(s2, entity_type::null_entity_id);
    ASSERT_TRUE(arch.empty());
    ASSERT_EQ(arch.size(), 0);
}

// ============================================================================
// Tick - get_tick returns a mutable reference into the tick pool
// ============================================================================
TEST(Archtype, Tick) {
    using entity_type = basic_entity<uint32_t, uint16_t>;
    using archtype_type = archtype<entity_type>;

    const ::myth::core::type_info* infos[] = { &INFO_POS };
    archtype_type arch(infos);

    Position p { 1.0f, 2.0f };
    void* datas[] = { &p };

    ASSERT_EQ(arch.emplace_back(10u, std::span<void*>(datas)), 0);

    // The reference is writable and the mutation is observed on read-back.
    ASSERT_EQ(arch.get_tick(0, 0), 0u);
    arch.get_tick(0, 0) = 7u;
    ASSERT_EQ(arch.get_tick(0, 0), 7u);

    // Two more entities, each tagged with a distinct tick, so a later erase can be
    // observed to move ticks in lockstep with the component data.
    ASSERT_EQ(arch.emplace_back(11u, std::span<void*>(datas)), 1);
    ASSERT_EQ(arch.emplace_back(12u, std::span<void*>(datas)), 2);
    arch.get_tick(0, 1) = 8u;
    arch.get_tick(0, 2) = 9u;

    // Erase compacts the tick pool alongside the component pool: erasing slot 0
    // swaps the last entity (12, tick 9) into it, so the tick must travel with it -
    // slot 0 now reads 9, not the erased entity's old 7. Slot 1 (entity 11) is
    // untouched, and the pool has shrunk so no stale tail tick lingers.
    ASSERT_EQ(arch.erase(0), 12u);
    ASSERT_EQ(arch.size(), 2);
    ASSERT_EQ(arch.get_tick(0, 0), 9u);
    ASSERT_EQ(arch.get_tick(0, 1), 8u);
}