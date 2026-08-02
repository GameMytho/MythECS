#pragma once

#include <span>
#include <vector>
#include <utility>
#include <cstdint>

#include "core/type_info.hpp"
#include "core/container/any_vector.hpp"

using ::myth::core::container::any_vector;

namespace myth::storage {
    /**
     * @brief A pure data container that stores entities and their component data in packed pools.
     *
     * An archetype owns the entity IDs, component pools (one `any_vector` per data component), and tick pools
     * for a single component-type combination. It does not know *which* component types it stores - the mapping
     * from component ID to pool index is maintained by the caller. All pool-indexed access
     * operations (`emplace_back`, `get_data`, `get_tick`) require the caller to supply data in pool order.
     *
     * @tparam EntityType The type of the entities managed by the archetype, which must have an `entity_id_type`
     * and a `null_entity_id`.
     * @tparam Allocator  The allocator type used for memory management of the underlying data structures
     * (default: `std::allocator`).
     */
    template<
        typename EntityType,
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
        /** @brief The type used to represent the size of the container. */
        using size_type = typename entity_ids_type::size_type;
        /** @brief The type of a single component pool - a type-erased array for one component type. */
        using component_set_type = any_vector<Allocator>;
        /** @brief The type used to store component pools. */
        using component_pool_type = std::vector<component_set_type, Allocator<component_set_type>>;
        /** @brief The type used to store tick values. */
        using tick_type = uint64_t;
        /** @brief The type used to store tick sets. */
        using tick_set_type = std::vector<tick_type, Allocator<tick_type>>;
        /** @brief The type used to store tick pools. */
        using tick_pool_type = std::vector<tick_set_type, Allocator<tick_set_type>>;

        /**
         * @brief Constructs an archetype with one component pool (and matching tick pool) per entry in @p infos.
         *
         * Each `type_info*` must correspond to a non-empty data component. The pools are created in the same
         * order as @p infos, establishing the pool-index layout that `emplace_back`, `get_data`, and `get_tick`
         * depend on. Tags (empty types) are excluded by the caller - they have no pool and no tick data.
         *
         * @param infos The type information for each data component, in pool-index order.
         */
        archtype(std::span<const ::myth::core::type_info*> infos)
            : _entity_ids() {
            size_type n = infos.size();

            _component_pool.reserve(n);
            _tick_pool.reserve(n);

            for (size_type i = 0; i < n; ++i) {
                _component_pool.emplace_back(*infos[i]);
                _tick_pool.emplace_back();
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
         * The @p datas span must contain one `void*` per pool, in the same order as the pools were created.
         * Each pointer is passed to the corresponding `any_vector::emplace_back` to copy-construct the component
         * instance, and a new tick entry is initialised to zero.
         *
         * @param eid   The ID of the entity to emplace.
         * @param datas One `void*` per data component pool, in pool-index order.
         *
         * @return The index of the emplaced entity in the archetype's internal storage.
         *
         * @warning The caller must ensure @p datas has one entry per pool (matching the layout established
         * at construction) and that each pointer points to a valid instance of the correct type.
         */
        [[nodiscard]] size_type emplace_back(entity_id_type eid, std::span<void*> datas) {
            _entity_ids.emplace_back(eid);

            for (size_type n = _component_pool.size(), i = 0; i < n; ++i) {
                _component_pool[i].emplace_back(datas[i]);
                _tick_pool[i].emplace_back(0u);
            }

            return _entity_ids.size() - 1;
        }

        /**
         * @brief Removes an entity from the archetype at the specified index.
         * 
         * Removes the entity and its associated component data from the archetype, by moving the last entity
         * in the storage to the specified index and popping the last entity. This operation maintains the
         * integrity of the internal storage and ensures that the archetype remains compact.
         * 
         * @param idx The index of the entity to remove.
         *
         * @return The ID of the swapped entity, or `entity_type::null_entity_id` if the removed entity was the
         * last one.
         *
         * @warning The archetype must not be empty and `idx` must be a valid entity index (`idx < size()`);
         * otherwise the behavior is undefined (an empty archetype underflows the last-index computation).
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
         * @brief Gets a pointer to the component data at the given pool and entity indices.
         *
         * @param col_idx The pool index (column), in the order the pools were created.
         * @param row_idx The entity index (row) within the archetype.
         *
         * @return A `void*` to the component data. The caller is responsible for casting to the correct type.
         */
        [[nodiscard]] void* get_data(size_type col_idx, size_type row_idx) noexcept {
            return _component_pool[col_idx][row_idx];
        }

        /**
         * @brief Gets the tick value at the given pool and entity indices.
         *
         * @param col_idx The pool index (column), in the order the pools were created.
         * @param row_idx The entity index (row) within the archetype.
         *
         * @return A mutable reference to the tick value.
         */
        [[nodiscard]] tick_type& get_tick(size_type col_idx, size_type row_idx) noexcept {
            return _tick_pool[col_idx][row_idx];
        }

    public:
        /** @brief Checks if the archetype is empty. */
        [[nodiscard]] bool empty() const noexcept { return _entity_ids.empty(); }

        /** @brief Returns the number of entities in the archetype. */
        [[nodiscard]] size_type size() const noexcept { return _entity_ids.size(); }

        /** @brief Returns the capacity of the archetype. */
        [[nodiscard]] size_type capacity() const noexcept { return _entity_ids.capacity(); }

        [[nodiscard]] size_type column() const noexcept { return _component_pool.size(); }

    private:
        entity_ids_type _entity_ids;
        component_pool_type _component_pool;
        tick_pool_type _tick_pool;
    };
} // namespace myth::storage