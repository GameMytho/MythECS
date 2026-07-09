#include <cstdint>
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
    // ECS code registers components. Tag is an empty type, so its type_info reports
    // `_empty` and the archetype treats it as a data-less tag.
    struct Position { float x; float y; };   // data component
    struct Velocity { float dx; float dy; }; // data component
    struct Tag {};                           // empty tag component (carries no data)
    struct Absent { float _pad; };           // a component the archetype never owns

    // Component ids and type_info records live together at namespace scope, keyed by
    // component type. The ids are assigned lazily by the generator - their values are
    // unspecified (not necessarily contiguous) but stable and distinct per type; the
    // tests never assume a particular numeric value or pool index. The type_info
    // references bind to the generator's cached records, which live for the whole
    // program and so safely outlive every archetype that borrows them.
    const uint32_t CID_POS    = ::myth::core::type_info_generator::id<Position>();
    const uint32_t CID_VEL    = ::myth::core::type_info_generator::id<Velocity>();
    const uint32_t CID_TAG    = ::myth::core::type_info_generator::id<Tag>();
    const uint32_t CID_ABSENT = ::myth::core::type_info_generator::id<Absent>();

    const ::myth::core::type_info& INFO_POS = ::myth::core::type_info_generator::info<Position>();
    const ::myth::core::type_info& INFO_VEL = ::myth::core::type_info_generator::info<Velocity>();
    const ::myth::core::type_info& INFO_TAG = ::myth::core::type_info_generator::info<Tag>();
}

// ============================================================================
// Functionalities - construct, emplace, get_data, get_tick, erase lifecycle
// ============================================================================
TEST(Archtype, Functionalities) {
    using entity_type = basic_entity<uint32_t, uint16_t>;
    using archtype_type = archtype<entity_type, uint32_t>;

    // Two data components and one tag, all supplied through `infos`. The tag is
    // listed first on purpose: the constructor still front-packs the data
    // components, so data access stays independent of the order within `infos`.
    archtype_type::meta_infos_type infos;
    infos.emplace_back(CID_TAG, INFO_TAG);
    infos.emplace_back(CID_POS, INFO_POS);
    infos.emplace_back(CID_VEL, INFO_VEL);

    archtype_type arch(infos);

    // Freshly constructed: no entities yet.
    ASSERT_TRUE(arch.empty());
    ASSERT_EQ(arch.size(), 0);
    ASSERT_EQ(arch.capacity(), 0);

    // Emplace an entity carrying Position + Velocity data.
    Position p { 1.0f, 2.0f };
    Velocity v { 3.0f, 4.0f };

    archtype_type::meta_datas_type datas;
    datas.emplace_back(CID_POS, &p);
    datas.emplace_back(CID_VEL, &v);

    auto idx = arch.emplace_back(10u, datas); // 10u is the entity id.

    ASSERT_EQ(idx, 0);
    ASSERT_FALSE(arch.empty());
    ASSERT_EQ(arch.size(), 1);
    ASSERT_GE(arch.capacity(), 1);

    // Component data is copied into the pools and retrievable by (cid, index).
    ASSERT_EQ(static_cast<Position*>(arch.get_data(CID_POS, 0))->x, 1.0f);
    ASSERT_EQ(static_cast<Position*>(arch.get_data(CID_POS, 0))->y, 2.0f);
    ASSERT_EQ(static_cast<Velocity*>(arch.get_data(CID_VEL, 0))->dx, 3.0f);
    ASSERT_EQ(static_cast<Velocity*>(arch.get_data(CID_VEL, 0))->dy, 4.0f);

    // Ticks are initialized to zero for every component of a new entity.
    ASSERT_EQ(arch.get_tick(CID_POS, 0), 0u);
    ASSERT_EQ(arch.get_tick(CID_VEL, 0), 0u);

    // Erasing the only (last) entity empties the archetype and reports null.
    auto swapped = arch.erase(0);

    ASSERT_EQ(swapped, entity_type::null_entity_id);
    ASSERT_TRUE(arch.empty());
    ASSERT_EQ(arch.size(), 0);
}

// ============================================================================
// Constructors - data-only, tag-only, and duplicate-data-id deduplication
// ============================================================================
TEST(Archtype, Constructors) {
    using entity_type = basic_entity<uint32_t, uint16_t>;
    using archtype_type = archtype<entity_type, uint32_t>;
    using ids_type = archtype_type::component_ids_type;

    // --- Data components only (no tags) ---
    {
        archtype_type::meta_infos_type infos;
        infos.emplace_back(CID_POS, INFO_POS);

        archtype_type arch(infos);

        ASSERT_TRUE(arch.empty());

        Position p { 5.0f, 6.0f };
        archtype_type::meta_datas_type datas;
        datas.emplace_back(CID_POS, &p);

        ASSERT_EQ(arch.emplace_back(1u, datas), 0);
        ASSERT_EQ(arch.size(), 1);
        ASSERT_EQ(static_cast<Position*>(arch.get_data(CID_POS, 0))->x, 5.0f);

        // The full component set is exactly {POS}.
        ids_type q;
        ASSERT_TRUE(q.emplace_back(CID_POS));
        ASSERT_TRUE(arch.include(q));
        ASSERT_TRUE(arch.matched(q));
    }

    // --- Tag components only (no data) ---
    {
        archtype_type::meta_infos_type infos;
        infos.emplace_back(CID_TAG, INFO_TAG);

        archtype_type arch(infos);

        ASSERT_TRUE(arch.empty());

        // An entity with no component data can still be stored.
        archtype_type::meta_datas_type datas;   // empty
        ASSERT_EQ(arch.emplace_back(1u, datas), 0);
        ASSERT_EQ(arch.size(), 1);

        // A tag is a member of the archetype's component set (even though it owns no data).
        ids_type q;
        ASSERT_TRUE(q.emplace_back(CID_TAG));
        ASSERT_TRUE(arch.include(q));
        ASSERT_TRUE(arch.matched(q));
    }

    // --- Duplicate data ids are deduplicated by the constructor ---
    {
        // Position is supplied twice; the second occurrence must be dropped so the
        // pool layout stays aligned with the distinct data ids {POS, VEL}.
        archtype_type::meta_infos_type infos;
        infos.emplace_back(CID_POS, INFO_POS);
        infos.emplace_back(CID_POS, INFO_POS);
        infos.emplace_back(CID_VEL, INFO_VEL);

        archtype_type arch(infos);

        Position p { 10.0f, 11.0f };
        Velocity v { 12.0f, 13.0f };
        archtype_type::meta_datas_type datas;
        datas.emplace_back(CID_POS, &p);
        datas.emplace_back(CID_VEL, &v);

        ASSERT_EQ(arch.emplace_back(1u, datas), 0);
        ASSERT_EQ(static_cast<Position*>(arch.get_data(CID_POS, 0))->x, 10.0f);
        ASSERT_EQ(static_cast<Velocity*>(arch.get_data(CID_VEL, 0))->dx, 12.0f);

        // Exactly two distinct data components remain after dedup.
        ids_type exact;
        ASSERT_TRUE(exact.emplace_back(CID_POS));
        ASSERT_TRUE(exact.emplace_back(CID_VEL));
        ASSERT_TRUE(arch.matched(exact));

        ids_type triple;
        ASSERT_TRUE(triple.emplace_back(CID_POS));
        ASSERT_TRUE(triple.emplace_back(CID_VEL));
        ASSERT_TRUE(triple.emplace_back(CID_ABSENT));
        ASSERT_FALSE(arch.matched(triple));
    }
}

// ============================================================================
// Move - move ctor and move assignment transfer contents; source left empty
// ============================================================================
TEST(Archtype, Move) {
    using entity_type = basic_entity<uint32_t, uint16_t>;
    using archtype_type = archtype<entity_type, uint32_t>;

    archtype_type::meta_infos_type infos1;
    infos1.emplace_back(CID_POS, INFO_POS);

    archtype_type arch1(infos1);

    Position p { 42.0f, 43.0f };
    archtype_type::meta_datas_type datas;
    datas.emplace_back(CID_POS, &p);

    ASSERT_EQ(arch1.emplace_back(10u, datas), 0);
    ASSERT_EQ(arch1.size(), 1);

    // Move ctor - contents transfer to arch2, arch1 is left empty.
    archtype_type arch2(std::move(arch1));

    ASSERT_TRUE(arch1.empty());
    ASSERT_EQ(arch1.size(), 0);

    ASSERT_EQ(arch2.size(), 1);
    ASSERT_EQ(static_cast<Position*>(arch2.get_data(CID_POS, 0))->x, 42.0f);
    ASSERT_EQ(static_cast<Position*>(arch2.get_data(CID_POS, 0))->y, 43.0f);
    ASSERT_EQ(arch2.get_tick(CID_POS, 0), 0u);

    // A second, independently populated archetype to receive a move assignment.
    archtype_type::meta_infos_type infos3;
    infos3.emplace_back(CID_POS, INFO_POS);

    archtype_type arch3(infos3);

    Position q { 7.0f, 8.0f };
    archtype_type::meta_datas_type datas3;
    datas3.emplace_back(CID_POS, &q);

    ASSERT_EQ(arch3.emplace_back(11u, datas3), 0);
    ASSERT_EQ(arch3.size(), 1);

    // Move assignment - arch3 takes over arch2's contents, arch2 is left empty.
    arch3 = std::move(arch2);

    ASSERT_TRUE(arch2.empty());
    ASSERT_EQ(arch2.size(), 0);

    ASSERT_EQ(arch3.size(), 1);
    ASSERT_EQ(static_cast<Position*>(arch3.get_data(CID_POS, 0))->x, 42.0f);
    ASSERT_EQ(static_cast<Position*>(arch3.get_data(CID_POS, 0))->y, 43.0f);
}

// ============================================================================
// Emplace - multiple entities packed contiguously with zero-initialized ticks
// ============================================================================
TEST(Archtype, Emplace) {
    using entity_type = basic_entity<uint32_t, uint16_t>;
    using archtype_type = archtype<entity_type, uint32_t>;

    archtype_type::meta_infos_type infos;
    infos.emplace_back(CID_POS, INFO_POS);
    infos.emplace_back(CID_VEL, INFO_VEL);

    archtype_type arch(infos);

    Position p0 { 10.0f, 11.0f }, p1 { 20.0f, 21.0f }, p2 { 30.0f, 31.0f };
    Velocity v0 { 1.0f, 2.0f }, v1 { 3.0f, 4.0f }, v2 { 5.0f, 6.0f };

    archtype_type::meta_datas_type d0, d1, d2;
    d0.emplace_back(CID_POS, &p0); d0.emplace_back(CID_VEL, &v0);
    d1.emplace_back(CID_POS, &p1); d1.emplace_back(CID_VEL, &v1);
    d2.emplace_back(CID_POS, &p2); d2.emplace_back(CID_VEL, &v2);

    // Entities are appended and report their packed index in insertion order.
    ASSERT_EQ(arch.emplace_back(10u, d0), 0);
    ASSERT_EQ(arch.emplace_back(11u, d1), 1);
    ASSERT_EQ(arch.emplace_back(12u, d2), 2);

    ASSERT_EQ(arch.size(), 3);

    // Every slot holds the data it was emplaced with, across both pools.
    ASSERT_EQ(static_cast<Position*>(arch.get_data(CID_POS, 0))->x, 10.0f);
    ASSERT_EQ(static_cast<Position*>(arch.get_data(CID_POS, 1))->x, 20.0f);
    ASSERT_EQ(static_cast<Position*>(arch.get_data(CID_POS, 2))->x, 30.0f);
    ASSERT_EQ(static_cast<Velocity*>(arch.get_data(CID_VEL, 0))->dx, 1.0f);
    ASSERT_EQ(static_cast<Velocity*>(arch.get_data(CID_VEL, 1))->dx, 3.0f);
    ASSERT_EQ(static_cast<Velocity*>(arch.get_data(CID_VEL, 2))->dx, 5.0f);

    // Ticks start at zero for each emplaced entity/component.
    ASSERT_EQ(arch.get_tick(CID_POS, 0), 0u);
    ASSERT_EQ(arch.get_tick(CID_POS, 1), 0u);
    ASSERT_EQ(arch.get_tick(CID_POS, 2), 0u);
    ASSERT_EQ(arch.get_tick(CID_VEL, 0), 0u);
    ASSERT_EQ(arch.get_tick(CID_VEL, 1), 0u);
    ASSERT_EQ(arch.get_tick(CID_VEL, 2), 0u);

    // Data for a component the archetype does not own is silently ignored, while
    // the recognized components are still stored at the new slot.
    Position p3 { 40.0f, 41.0f };
    Velocity v3 { 7.0f, 8.0f };
    Position foreign { -1.0f, -1.0f };

    archtype_type::meta_datas_type d3;
    d3.emplace_back(CID_POS, &p3);
    d3.emplace_back(CID_ABSENT, &foreign);   // not part of the archetype - dropped
    d3.emplace_back(CID_VEL, &v3);

    ASSERT_EQ(arch.emplace_back(13u, d3), 3);
    ASSERT_EQ(arch.size(), 4);
    ASSERT_EQ(static_cast<Position*>(arch.get_data(CID_POS, 3))->x, 40.0f);
    ASSERT_EQ(static_cast<Velocity*>(arch.get_data(CID_VEL, 3))->dx, 7.0f);
}

// ============================================================================
// Erase - swap-and-pop compaction; returns the id of the swapped-in entity
// ============================================================================
TEST(Archtype, Erase) {
    using entity_type = basic_entity<uint32_t, uint16_t>;
    using archtype_type = archtype<entity_type, uint32_t>;

    archtype_type::meta_infos_type infos;
    infos.emplace_back(CID_POS, INFO_POS);

    archtype_type arch(infos);

    // Three entities (ids 10/11/12) with distinguishable positions.
    Position p0 { 10.0f, 0.0f }, p1 { 20.0f, 0.0f }, p2 { 30.0f, 0.0f };

    archtype_type::meta_datas_type d0, d1, d2;
    d0.emplace_back(CID_POS, &p0);
    d1.emplace_back(CID_POS, &p1);
    d2.emplace_back(CID_POS, &p2);

    ASSERT_EQ(arch.emplace_back(10u, d0), 0);
    ASSERT_EQ(arch.emplace_back(11u, d1), 1);
    ASSERT_EQ(arch.emplace_back(12u, d2), 2);
    ASSERT_EQ(arch.size(), 3);

    // Erase the first slot: the last entity (12) is swapped into index 0, so its
    // id is returned to let the caller patch its mapping.
    auto s0 = arch.erase(0);

    ASSERT_EQ(s0, 12u);
    ASSERT_EQ(arch.size(), 2);
    ASSERT_EQ(static_cast<Position*>(arch.get_data(CID_POS, 0))->x, 30.0f);  // entity 12 moved here
    ASSERT_EQ(static_cast<Position*>(arch.get_data(CID_POS, 1))->x, 20.0f);  // entity 11 unchanged

    // Erase the last slot: nothing is moved, so null is returned.
    auto s1 = arch.erase(1);

    ASSERT_EQ(s1, entity_type::null_entity_id);
    ASSERT_EQ(arch.size(), 1);
    ASSERT_EQ(static_cast<Position*>(arch.get_data(CID_POS, 0))->x, 30.0f);

    // Erase the remaining (only, hence last) entity: archetype becomes empty.
    auto s2 = arch.erase(0);

    ASSERT_EQ(s2, entity_type::null_entity_id);
    ASSERT_TRUE(arch.empty());
    ASSERT_EQ(arch.size(), 0);
}

// ============================================================================
// IncludeAndMatched - membership subset query vs. exact full-set query
// ============================================================================
TEST(Archtype, IncludeAndMatched) {
    using entity_type = basic_entity<uint32_t, uint16_t>;
    using archtype_type = archtype<entity_type, uint32_t>;
    using ids_type = archtype_type::component_ids_type;

    archtype_type::meta_infos_type infos;
    infos.emplace_back(CID_POS, INFO_POS);
    infos.emplace_back(CID_VEL, INFO_VEL);
    infos.emplace_back(CID_TAG, INFO_TAG);

    archtype_type arch(infos);

    // include() checks membership in the archetype's full component set (data + tags).
    ids_type q_pos;
    ASSERT_TRUE(q_pos.emplace_back(CID_POS));
    ASSERT_TRUE(arch.include(q_pos));

    ids_type q_pos_vel;
    ASSERT_TRUE(q_pos_vel.emplace_back(CID_POS));
    ASSERT_TRUE(q_pos_vel.emplace_back(CID_VEL));
    ASSERT_TRUE(arch.include(q_pos_vel));

    ids_type q_tag;   // a tag is a member of the component set too
    ASSERT_TRUE(q_tag.emplace_back(CID_TAG));
    ASSERT_TRUE(arch.include(q_tag));

    ids_type q_absent;
    ASSERT_TRUE(q_absent.emplace_back(CID_POS));
    ASSERT_TRUE(q_absent.emplace_back(CID_ABSENT));
    ASSERT_FALSE(arch.include(q_absent));

    // matched() requires the exact union of tags and data components.
    ids_type m_full;
    ASSERT_TRUE(m_full.emplace_back(CID_POS));
    ASSERT_TRUE(m_full.emplace_back(CID_VEL));
    ASSERT_TRUE(m_full.emplace_back(CID_TAG));
    ASSERT_TRUE(arch.matched(m_full));

    ids_type m_partial;   // right members, wrong count
    ASSERT_TRUE(m_partial.emplace_back(CID_POS));
    ASSERT_TRUE(m_partial.emplace_back(CID_VEL));
    ASSERT_FALSE(arch.matched(m_partial));

    ids_type m_wrong;     // right count, but one member is foreign
    ASSERT_TRUE(m_wrong.emplace_back(CID_POS));
    ASSERT_TRUE(m_wrong.emplace_back(CID_VEL));
    ASSERT_TRUE(m_wrong.emplace_back(CID_ABSENT));
    ASSERT_FALSE(arch.matched(m_wrong));
}

// ============================================================================
// Tick - get_tick returns a mutable reference into the tick pool
// ============================================================================
TEST(Archtype, Tick) {
    using entity_type = basic_entity<uint32_t, uint16_t>;
    using archtype_type = archtype<entity_type, uint32_t>;

    archtype_type::meta_infos_type infos;
    infos.emplace_back(CID_POS, INFO_POS);

    archtype_type arch(infos);

    Position p { 1.0f, 2.0f };
    archtype_type::meta_datas_type datas;
    datas.emplace_back(CID_POS, &p);

    ASSERT_EQ(arch.emplace_back(10u, datas), 0);

    // The reference is writable and the mutation is observed on read-back.
    ASSERT_EQ(arch.get_tick(CID_POS, 0), 0u);
    arch.get_tick(CID_POS, 0) = 7u;
    ASSERT_EQ(arch.get_tick(CID_POS, 0), 7u);

    // Two more entities, each tagged with a distinct tick, so a later erase can be
    // observed to move ticks in lockstep with the component data.
    ASSERT_EQ(arch.emplace_back(11u, datas), 1);
    ASSERT_EQ(arch.emplace_back(12u, datas), 2);
    arch.get_tick(CID_POS, 1) = 8u;
    arch.get_tick(CID_POS, 2) = 9u;

    // Erase compacts the tick pool alongside the component pool: erasing slot 0
    // swaps the last entity (12, tick 9) into it, so the tick must travel with it -
    // slot 0 now reads 9, not the erased entity's old 7. Slot 1 (entity 11) is
    // untouched, and the pool has shrunk so no stale tail tick lingers.
    ASSERT_EQ(arch.erase(0), 12u);
    ASSERT_EQ(arch.size(), 2);
    ASSERT_EQ(arch.get_tick(CID_POS, 0), 9u);
    ASSERT_EQ(arch.get_tick(CID_POS, 1), 8u);
}

// ============================================================================
// DeriveExtend - derive(infos) grows the component set; source is untouched
// ============================================================================
TEST(Archtype, DeriveExtend) {
    using entity_type = basic_entity<uint32_t, uint16_t>;
    using archtype_type = archtype<entity_type, uint32_t>;
    using ids_type = archtype_type::component_ids_type;

    // Source archetype {POS} holding one live entity.
    archtype_type::meta_infos_type infos;
    infos.emplace_back(CID_POS, INFO_POS);

    archtype_type arch(infos);

    Position sp { 1.0f, 2.0f };
    archtype_type::meta_datas_type sdatas;
    sdatas.emplace_back(CID_POS, &sp);
    ASSERT_EQ(arch.emplace_back(10u, sdatas), 0);
    ASSERT_EQ(arch.size(), 1);

    // Derive a neighbour that adds a data component (VEL) and a tag (TAG). POS is
    // supplied again to prove an already-owned id is ignored, not duplicated.
    archtype_type::meta_infos_type add;
    add.emplace_back(CID_VEL, INFO_VEL);
    add.emplace_back(CID_TAG, INFO_TAG);
    add.emplace_back(CID_POS, INFO_POS);   // already present - dropped

    archtype_type derived = arch.derive(add);

    // The derived set is exactly {POS, VEL, TAG}.
    ids_type full;
    ASSERT_TRUE(full.emplace_back(CID_POS));
    ASSERT_TRUE(full.emplace_back(CID_VEL));
    ASSERT_TRUE(full.emplace_back(CID_TAG));
    ASSERT_TRUE(derived.matched(full));

    // The derived archetype starts empty - it inherits the set, never the entities.
    ASSERT_TRUE(derived.empty());
    ASSERT_EQ(derived.size(), 0);

    // The source is left untouched by the const derive.
    ASSERT_EQ(arch.size(), 1);
    ASSERT_EQ(static_cast<Position*>(arch.get_data(CID_POS, 0))->x, 1.0f);

    // The derived pools are independent and fully usable: both data components store
    // and retrieve correctly, and ticks start at zero.
    Position dp { 3.0f, 4.0f };
    Velocity dv { 5.0f, 6.0f };
    archtype_type::meta_datas_type ddatas;
    ddatas.emplace_back(CID_POS, &dp);
    ddatas.emplace_back(CID_VEL, &dv);

    ASSERT_EQ(derived.emplace_back(20u, ddatas), 0);
    ASSERT_EQ(derived.size(), 1);
    ASSERT_EQ(static_cast<Position*>(derived.get_data(CID_POS, 0))->x, 3.0f);
    ASSERT_EQ(static_cast<Velocity*>(derived.get_data(CID_VEL, 0))->dx, 5.0f);
    ASSERT_EQ(derived.get_tick(CID_POS, 0), 0u);
    ASSERT_EQ(derived.get_tick(CID_VEL, 0), 0u);

    // Populating the derived archetype does not leak back into the source.
    ASSERT_EQ(arch.size(), 1);
}

// ============================================================================
// DeriveReduce - derive(ids) shrinks the set; survivors stay pool-aligned
// ============================================================================
TEST(Archtype, DeriveReduce) {
    using entity_type = basic_entity<uint32_t, uint16_t>;
    using archtype_type = archtype<entity_type, uint32_t>;
    using ids_type = archtype_type::component_ids_type;

    // Source archetype {POS, VEL, TAG}.
    archtype_type::meta_infos_type infos;
    infos.emplace_back(CID_POS, INFO_POS);
    infos.emplace_back(CID_VEL, INFO_VEL);
    infos.emplace_back(CID_TAG, INFO_TAG);

    archtype_type arch(infos);

    // --- Remove a tag: the data components are unaffected ---
    {
        ids_type drop;
        ASSERT_TRUE(drop.emplace_back(CID_TAG));

        archtype_type derived = arch.derive(drop);

        ids_type expect;
        ASSERT_TRUE(expect.emplace_back(CID_POS));
        ASSERT_TRUE(expect.emplace_back(CID_VEL));
        ASSERT_TRUE(derived.matched(expect));
        ASSERT_TRUE(derived.empty());
    }

    // --- Remove ids the archetype does not own: the set is unchanged ---
    {
        ids_type drop;
        ASSERT_TRUE(drop.emplace_back(CID_ABSENT));

        archtype_type derived = arch.derive(drop);

        ids_type expect;
        ASSERT_TRUE(expect.emplace_back(CID_POS));
        ASSERT_TRUE(expect.emplace_back(CID_VEL));
        ASSERT_TRUE(expect.emplace_back(CID_TAG));
        ASSERT_TRUE(derived.matched(expect));
    }

    // --- Remove a front data component: the survivor must re-pack to pool index 0 ---
    {
        // A two-data archetype {POS, VEL}; dropping POS (pool index 0) must leave VEL
        // repacked at pool index 0, so get_data(VEL) reads the freshly emplaced value
        // rather than reaching into a stale/absent slot.
        archtype_type::meta_infos_type two;
        two.emplace_back(CID_POS, INFO_POS);
        two.emplace_back(CID_VEL, INFO_VEL);

        archtype_type pv(two);

        ids_type drop;
        ASSERT_TRUE(drop.emplace_back(CID_POS));

        archtype_type derived = pv.derive(drop);

        ids_type expect;
        ASSERT_TRUE(expect.emplace_back(CID_VEL));
        ASSERT_TRUE(derived.matched(expect));

        Velocity v { 7.0f, 8.0f };
        archtype_type::meta_datas_type datas;
        datas.emplace_back(CID_VEL, &v);
        datas.emplace_back(CID_POS, &v);   // POS no longer owned - dropped

        ASSERT_EQ(derived.emplace_back(30u, datas), 0);
        ASSERT_EQ(static_cast<Velocity*>(derived.get_data(CID_VEL, 0))->dx, 7.0f);
    }
}

// ============================================================================
// DeriveIdentity - deriving with an empty delta reproduces the same set
// ============================================================================
TEST(Archtype, DeriveIdentity) {
    using entity_type = basic_entity<uint32_t, uint16_t>;
    using archtype_type = archtype<entity_type, uint32_t>;
    using ids_type = archtype_type::component_ids_type;

    archtype_type::meta_infos_type infos;
    infos.emplace_back(CID_POS, INFO_POS);
    infos.emplace_back(CID_TAG, INFO_TAG);

    archtype_type arch(infos);

    ids_type expect;
    ASSERT_TRUE(expect.emplace_back(CID_POS));
    ASSERT_TRUE(expect.emplace_back(CID_TAG));

    // Extending by nothing yields the same component set.
    archtype_type::meta_infos_type none_add;
    archtype_type extended = arch.derive(none_add);
    ASSERT_TRUE(extended.matched(expect));
    ASSERT_TRUE(extended.empty());

    // Reducing by nothing yields the same component set.
    ids_type none_drop;
    archtype_type reduced = arch.derive(none_drop);
    ASSERT_TRUE(reduced.matched(expect));
    ASSERT_TRUE(reduced.empty());
}
