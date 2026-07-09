#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <utility>
#include <cstdint>

#include "core/type_info.hpp"
#include "core/container/dense_set.hpp"
#include "core/container/any_vector.hpp"

using ::myth::core::container::dense_set;
using ::myth::core::container::any_vector;

namespace myth::storage {
    /**
     * @brief An archetype that represents a unique combination of component types and manages entities with those components.
     * 
     * The `archtype` class is a template that takes an `EntityType`, a `ComponentIdType`, and optional template parameters for
     * hash, equality, and allocator types. It manages a collection of entities that share the same set of component types,
     * allowing for efficient storage and retrieval of component data associated with those entities.
     * 
     * @tparam EntityType The type of the entities managed by the archetype, which must have an `entity_id_type` and a `null_entity_id`.
     * @tparam ComponentIdType The type used to identify component types, typically an integral or enum type.
     * @tparam Hash A unary functor that computes the hash of a component ID (default: `std::hash`).
     * @tparam Equal A binary functor that compares two component IDs for equality (default: `std::equal_to`).
     * @tparam Allocator The allocator type used for memory management of the underlying data structures (default: `std::allocator`).
     */
    template<
        typename EntityType,
        typename ComponentIdType,
        template<typename> typename Hash = std::hash,
        template<typename> typename Equal = std::equal_to,
        template<typename> typename Allocator = std::allocator
    >
    class archtype final {
    public:
        /** @brief The type of the entities managed by the archetype. */
        using entity_type = EntityType;
        /** @brief The type used to identify entities. */
        using entity_id_type = typename entity_type::entity_id_type;
        /** @brief The type used to store entity IDs. */
        using entity_ids_type = std::vector<entity_id_type, Allocator<entity_id_type>>;
        /** @brief The type used to identify component types. */
        using component_id_type = ComponentIdType;
        /** @brief The type used to store component IDs. */
        using component_ids_type = dense_set<component_id_type, Hash, Equal, Allocator>;
        /** @brief The type used to represent the size of the container. */
        using size_type = typename component_ids_type::size_type;
        /** @brief The type used to store component sets. */
        using component_set_type = any_vector<Allocator>;
        /** @brief The type used to store component pools. */
        using component_pool_type = std::vector<component_set_type, Allocator<component_set_type>>;
        /** @brief The type used to store tick values. */
        using tick_type = uint64_t;
        /** @brief The type used to store tick sets. */
        using tick_set_type = std::vector<tick_type, Allocator<tick_type>>;
        /** @brief The type used to store tick pools. */
        using tick_pool_type = std::vector<tick_set_type, Allocator<tick_set_type>>;

        /** @brief The type used to store metadata information about components. */
        using meta_info_type = std::pair<component_id_type, const ::myth::core::type_info&>;
        /** @brief The type used to store a collection of metadata information about components. */
        using meta_infos_type = std::vector<meta_info_type, Allocator<meta_info_type>>;
        /** @brief The type used to store metadata about component data. */
        using meta_data_type = std::pair<component_id_type, void*>;
        /** @brief The type used to store a collection of metadata about component data. */
        using meta_datas_type = std::vector<meta_data_type, Allocator<meta_data_type>>;

        /**
         * @brief Constructs an archetype from the metadata of all its components (data components and empty tags alike).
         *
         * Each entry is classified by its type information: a data component (non-empty type) gets a component pool and a
         * tick pool, whereas a tag (empty type) owns neither. As they are registered, data components are kept at the front
         * of `_component_ids` so that each one's index matches its pool index in `_component_pool` and `_tick_pool`; tags
         * settle after them. Duplicate ids are dropped. All ids together form the archetype's component set.
         *
         * @param infos The metadata of the components, each pairing a component id with its type information.
         */
        archtype(const meta_infos_type& infos)
            : _entity_ids() {
            size_type n = infos.size();

            _component_ids.reserve(n);
            _component_pool.reserve(n);
            _tick_pool.reserve(n);

            for (size_type i = 0, l = 0; i < n; ++i) {
                const meta_info_type& info = infos[i];
                if (!_component_ids.emplace_back(info.first)) {
                    continue;
                }

                if (!info.second._empty) {
                    if (size_type k = _component_ids.size() - 1; k != l) {
                        _component_ids.swap(k, l);
                    }

                    ++l;
                    _component_pool.emplace_back(info.second);
                    _tick_pool.emplace_back();
                }
            }
        }

        /** @brief Constructs an archetype by copying another archetype. */
        archtype(const archtype&) = delete;

        /** @brief Constructs an archetype by moving another archetype. */
        archtype(archtype&&) = default;

        /** @brief Destructs the archetype. */
        ~archtype() noexcept = default;

        /** @brief Assigns the contents of another archetype to this archetype. */
        archtype& operator=(const archtype&) = delete;

        /** @brief Moves the contents of another archetype to this archetype. */
        archtype& operator=(archtype&&) noexcept = default;

    public:
        /**
         * @brief Emplaces an entity into the archetype with the specified component data.
         * 
         * @param eid The ID of the entity to emplace.
         * @param datas The metadata about the component data associated with the entity.
         * 
         * @return The index of the emplaced entity in the archetype's internal storage.
         * 
         * @warning The caller must ensure that the provided component data matches the archetype's component IDs and types.
         */
        [[nodiscard]] size_type emplace_back(entity_id_type eid, const meta_datas_type& datas) {
            const size_type n = _component_pool.size();

            _entity_ids.emplace_back(eid);

            for (size_type m = datas.size(), i = 0; i < m; ++i) {
                const meta_data_type& data = datas[i];

                size_type idx = _component_ids.index(data.first);
                if (idx >= n) {
                    continue;
                }

                _component_pool[idx].emplace_back(data.second);
                _tick_pool[idx].emplace_back(0u);
            }

            return _entity_ids.size() - 1;
        }

        /**
         * @brief Removes an entity from the archetype at the specified index.
         * 
         * Removes the entity and its associated component data from the archetype, by moving the last entity in the storage to
         * the specified index and popping the last entity. This operation maintains the integrity of the internal storage and
         * ensures that the archetype remains compact.
         * 
         * @param idx The index of the entity to remove.
         *
         * @return The ID of the swapped entity, or `entity_type::null_entity_id` if the removed entity was the last one.
         *
         * @warning The archetype must not be empty and `idx` must be a valid entity index (`idx < size()`); otherwise the
         * behavior is undefined (an empty archetype underflows the last-index computation).
         */
        [[nodiscard]] entity_id_type erase(size_type idx) {
            size_type last = _entity_ids.size() - 1;

            if (idx != last) {
                std::swap(_entity_ids[idx], _entity_ids[last]);
                for (size_type n = _component_pool.size(), i = 0; i < n; ++i) {
                    _component_pool[i].swap(idx, last);
                    std::swap(_tick_pool[i][idx], _tick_pool[i][last]);
                }
            }

            _entity_ids.pop_back();
            for (size_type n = _component_pool.size(), i = 0; i < n; ++i) {
                _component_pool[i].pop_back();
                _tick_pool[i].pop_back();
            }

            return idx == last ? entity_type::null_entity_id : _entity_ids[idx];
        }

        /**
         * @brief Gets a pointer to the component data of the specified component type for the entity at the given index.
         *
         * @param cid The ID of the component type to retrieve data for.
         * @param idx The index of the entity in the archetype's internal storage.
         * 
         * @return A pointer to the component data associated with the entity at the specified index, it should be cast to the
         * appropriate type based on the component's type information.
         */
        [[nodiscard]] void* get_data(component_id_type cid, size_type idx) noexcept {
            size_type cidx = _component_ids.index(cid);
            return _component_pool[cidx][idx];
        }

        /**
         * @brief Gets the tick value associated with a component of an entity at the specified index in the archetype.
         * 
         * @param cid The ID of the component type to retrieve the tick value for.
         * @param idx The index of the entity in the archetype's internal storage.
         * 
         * @return A reference to the tick value associated with the component of the entity at the specified index.
         */
        [[nodiscard]] tick_type& get_tick(component_id_type cid, size_type idx) noexcept {
            size_type cidx = _component_ids.index(cid);
            return _tick_pool[cidx][idx];
        }

        /**
         * @brief Checks whether the archetype owns every one of the specified component IDs.
         *
         * The match is by containment: it holds as long as each given ID belongs to the archetype, which is
         * free to own further components beyond those listed. An empty set of IDs is trivially contained.
         *
         * @param ids The collection of component IDs the archetype is required to contain.
         *
         * @return True if every specified component ID belongs to the archetype, false otherwise.
         */
        [[nodiscard]] bool include(const component_ids_type& ids) const noexcept {
            for (size_type n = ids.size(), i = 0; i < n; ++i) {
                if (!_component_ids.contains(ids[i])) {
                    return false;
                }
            }

            return true;
        }

        /**
         * @brief Checks whether the archetype's component set is exactly the specified component IDs.
         *
         * The match is by exact equality: it holds only when the given IDs and the archetype's components form
         * the same set - identical members and identical count, with nothing extra on either side.
         *
         * @param ids The collection of component IDs to compare against the archetype's whole component set.
         *
         * @return True if the archetype's component set equals the specified component IDs, false otherwise.
         */
        [[nodiscard]] bool matched(const component_ids_type& ids) const noexcept {
            if (ids.size() != _component_ids.size()) {
                return false;
            }

            for (size_type n = ids.size(), i = 0; i < n; ++i) {
                if (!_component_ids.contains(ids[i])) {
                    return false;
                }
            }

            return true;
        }

        /**
         * @brief Derives a new archetype by extending this one's component set with additional components.
         *
         * The derived archetype owns every component of this one plus each new entry in `infos` (data components and
         * empty tags alike); ids already present are dropped so the set stays unique. Data components are kept at the
         * front of the derived component id set so that each one's index still matches its pool index. The derived
         * archetype starts with no entities: every pool is freshly built from the corresponding type information and
         * left empty, so this archetype is left untouched.
         *
         * @param infos The metadata of the components to add, each pairing a component id with its type information.
         *
         * @return A new archetype whose component set is this one's plus the given components.
         */
        [[nodiscard]] archtype derive(const meta_infos_type& infos) const {
            size_type n = _component_pool.size();
            size_type m = infos.size();

            component_ids_type new_ids(_component_ids);
            component_pool_type new_pool;

            new_ids.reserve(new_ids.size() + m);
            new_pool.reserve(n + m);

            for (size_type i = 0; i < n; ++i) {
                new_pool.emplace_back(_component_pool[i].info());
            }

            for (size_type i = 0, l = n; i < m; ++i) {
                const meta_info_type& info = infos[i];
                if (!new_ids.emplace_back(info.first)) {
                    continue;
                }

                if (info.second._empty) {
                    continue;
                }

                if (size_type k = new_ids.size() - 1; k != l) {
                    new_ids.swap(k, l);
                }

                ++l;
                new_pool.emplace_back(info.second);
            }

            return archtype(std::move(new_ids), std::move(new_pool));
        }

        /**
         * @brief Derives a new archetype by removing the specified components from this one's component set.
         *
         * The derived archetype owns every component of this one except those whose id appears in `ids`; ids not owned
         * by this archetype are simply absent and thus ignored. The surviving data components keep their relative order,
         * so that each one's index still matches its pool index. The derived archetype starts with no entities: every
         * retained pool is freshly built from the corresponding type information and left empty, so this archetype is
         * left untouched.
         *
         * @param ids The component ids to remove from the derived archetype.
         *
         * @return A new archetype whose component set is this one's minus the given components.
         */
        [[nodiscard]] archtype derive(const component_ids_type& ids) const {
            size_type n = _component_ids.size();
            size_type m = _component_pool.size();

            component_ids_type new_ids(n);
            component_pool_type new_pool;

            new_pool.reserve(m);

            for (size_type i = 0; i < n; ++i) {
                component_id_type id = _component_ids[i];
                if (ids.contains(id)) {
                    continue;
                }

                (void)new_ids.emplace_back(id);

                if (i < m) {
                    new_pool.emplace_back(_component_pool[i].info());
                }
            }

            return archtype(std::move(new_ids), std::move(new_pool));
        }

    public:
        /** @brief Checks if the archetype is empty. */
        [[nodiscard]] bool empty() const noexcept { return _entity_ids.empty(); }

        /** @brief Returns the number of entities in the archetype. */
        [[nodiscard]] size_type size() const noexcept { return _entity_ids.size(); }

        /** @brief Returns the capacity of the archetype. */
        [[nodiscard]] size_type capacity() const noexcept { return _entity_ids.capacity(); }

    private:
        component_ids_type _component_ids;
        entity_ids_type _entity_ids;
        component_pool_type _component_pool;
        tick_pool_type _tick_pool;

    private:
        /**
         * @brief Constructs an empty archetype from a ready-made component id set and its component pools.
         *
         * Used by `derive` to assemble a neighbouring archetype: the component id set and its (empty) pools are handed
         * over directly, no entities are present yet, and one empty tick set is created per component pool so the tick
         * pool stays aligned with the component pool.
         *
         * @param ids The component id set, with data components front-packed to match the pools.
         * @param pool The component pools, one per data component, each already built from its type information.
         */
        archtype(component_ids_type&& ids, component_pool_type&& pool)
            : _component_ids(std::move(ids)), _entity_ids(), _component_pool(std::move(pool)),
            _tick_pool(_component_pool.size()) {}
    };
} // namespace myth::storage