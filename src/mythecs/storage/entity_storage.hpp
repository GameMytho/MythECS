#pragma once

#include <cstddef>
#include <memory>
#include <vector>
#include <utility>

#include "storage/entity_set.hpp"

namespace myth::storage {
    /**
     * @brief A hybrid storage for entities and their associated values.
     *
     * This class manages a collection of entities together with their associated values using packed storage.
     * Entities are stored in a contiguous region internally partitioned into three zones:
     *   - [0, values.size()):  entities that have an associated value.
     *   - [values.size(), count): spawned entities that do not yet have a value.
     *   - [count, entities.size()): dead entity slots available for reuse by spawn().
     * The underlying sparse set enables O(1) lookup by entity ID, while the packed layout keeps value iteration
     * cache-friendly.
     *
     * @tparam EntityType The type of the entities stored, which must have an entity_id_type and entity_version_type defined.
     * @tparam ValueType The type of the values associated with entities.
     * @tparam Allocator The allocator type used for memory management of the underlying data structures. Defaults to
     * std::allocator.
     * @tparam CacheLineSize The size of a cache line in bytes, used for cache-line alignment in the sparse set.
     * @tparam PageSize The number of entries in each page of the sparse set.
     */
    template<
        typename EntityType,
        typename ValueType,
        template<typename> typename Allocator = std::allocator,
        size_t CacheLineSize = 64,
        size_t PageSize = 256
    >
    class entity_storage final {
    public:
        /** @brief The type of the entities stored. */
        using entity_type = EntityType;
        /** @brief The type of the entity set used internally. */
        using entities_type = entity_set<entity_type, Allocator, CacheLineSize, PageSize>;
        /** @brief The type used for sizes and indices in the storage. */
        using size_type = typename entities_type::size_type;
        /** @brief The type used for indices of entities in the storage. */
        using entity_index_type = typename entities_type::entity_index_type;
        /** @brief The type of the values. */
        using value_type = ValueType;
        /** @brief The type of the vector storing values. */
        using values_type = std::vector<value_type, Allocator<value_type>>;

        /** @brief A special value indicating an invalid index in the entity storage. */
        inline static constexpr entity_index_type null_entity_index = entities_type::null_entity_index;

        /** @brief Constructs an empty entity storage. */
        entity_storage() noexcept = default;

        /**
         * @brief Constructs an entity storage with the specified initial capacity.
         *
         * @param n The initial capacity for both entities and values.
         */
        entity_storage(size_type n) {
            _entities.reserve(n);
            _values.reserve(n);
        }

        /** @brief Constructs an entity storage by copying another one. */
        entity_storage(const entity_storage&) = default;

        /** @brief Constructs an entity storage by moving another one. */
        entity_storage(entity_storage&& s) noexcept {
            _entities = std::move(s._entities);
            _values = std::move(s._values);
            _count = s._count;
            s._count = 0;
        }

        /** @brief Destructs the entity storage. */
        ~entity_storage() noexcept = default;

        /** @brief Assigns another entity storage to this one. */
        entity_storage& operator=(const entity_storage&) = default;

        /** @brief Moves another entity storage to this one. */
        entity_storage& operator=(entity_storage&& s) noexcept {
            _entities = std::move(s._entities);
            _values = std::move(s._values);
            _count = s._count;
            s._count = 0;

            return *this;
        }

    public:
        /**
         * @brief Spawns a new entity.
         *
         * Reuses a previously despawned entity slot if one is available, otherwise creates a brand-new entity
         * in the underlying entity set. The returned entity is alive and ready to receive a value
         * via emplace().
         *
         * @return The spawned entity.
         */
        [[nodiscard]] const entity_type spawn() {
            size_type size = _entities.size();

            if (_count < size) {
                return _entities[_count++];
            }

            entity_type e(size);

            if (e.valid()) {
                _entities.emplace_back(e);
                ++_count;
            }

            return e;
        }

        /**
         * @brief Despawns an entity, marking its slot as available for reuse.
         *
         * The entity is moved to the dead zone (beyond _count) and its version is incremented so that any
         * stale references to it become invalid. The slot can later be reclaimed by a subsequent spawn().
         *
         * @param entt The entity to despawn.
         *
         * @warning The entity must not have an associated value when this function is called, otherwise the
         * behavior is undefined. Call erase() first to remove the value if one exists.
         */
        void despawn(const entity_type& entt) noexcept {
            size_type idx = _entities.index(entt);

            --_count;
            if (idx != _count) {
                _entities.swap(idx, _count);
            }

            _entities.version_next(_count);
        }

        /**
         * @brief Associates a value with an entity.
         *
         * The entity is swapped into the packed values zone so that _values[i] always corresponds to
         * _entities[i] for all i < _values.size().
         *
         * @param entt The entity to associate the value with.
         * @param value The component value to associate.
         *
         * @warning The entity must be spawned and must not already have an associated value, otherwise the
         * behavior is undefined.
         */
        void emplace(const entity_type& entt, const value_type& value) {
            size_type idx = _entities.index(entt);
            size_type size = _values.size();

            if (idx != size) {
                _entities.swap(idx, size);
            }

            _values.push_back(value);
        }

        /**
         * @brief Removes the associated value from an entity.
         *
         * Uses swap-and-pop to keep the values vector tightly packed. The entity itself remains spawned
         * and can later receive a new value via emplace().
         *
         * @param entt The entity whose value to remove.
         *
         * @warning The entity must have an associated value, otherwise the behavior is undefined.
         */
        void erase(const entity_type& entt) noexcept {
            size_type idx = _entities.index(entt);
            size_type last_idx = _values.size() - 1;

            if (idx != last_idx) {
                _entities.swap(idx, last_idx);
                std::swap(_values[idx], _values[last_idx]);
            }

            _values.pop_back();
        }

        /**
         * @brief Gets the index of an entity in the storage.
         *
         * @param entt The entity to find.
         * 
         * @return The index of the entity if found.
         * 
         * @warning Before calling this function, ensure that the entity exists in the storage by using the
         * contains() function, otherwise the behavior is undefined.
         */
        [[nodiscard]] entity_index_type index(const entity_type& entt) const noexcept {
            return _entities.index(entt);
        }

        /**
         * @brief Gets the index of an entity in the storage.
         *
         * @param entt The entity to find.
         * 
         * @return The index of the entity, or null_entity_index if not found.
         */
        [[nodiscard]] entity_index_type checked_index(const entity_type& entt) const noexcept {
            entity_index_type index = _entities.checked_index(entt);
            if (index < _count) {
                return index;
            }

            return null_entity_index;
        }

        /**
         * @brief Checks if an entity is contained in the storage (spawned but may not have a value).
         *
         * @param entt The entity to check.
         * 
         * @return True if the entity is spawned, false otherwise.
         */
        [[nodiscard]] bool contains(const entity_type& entt) const noexcept {
            entity_index_type index = _entities.checked_index(entt);
            return index != null_entity_index && index < _count;
        }

        /**
         * @brief Checks if an entity is alive (has an associated value).
         *
         * @param entt The entity to check.
         * 
         * @return True if the entity has an associated value, false otherwise.
         */
        [[nodiscard]] bool alive(const entity_type& entt) const noexcept {
            entity_index_type index = _entities.checked_index(entt);
            return index != null_entity_index && index < _values.size();
        }

        /**
         * @brief Clears all entities and values, resetting the storage to empty.
         */
        void clear() noexcept {
            _entities.clear();
            _values.clear();
            _count = 0;
        }

        /**
         * @brief Shrinks the capacity of the internal storage to fit the current size.
         */
        void shrink_to_fit() {
            _entities.shrink_to_fit();
            _values.shrink_to_fit();
        }

        /**
         * @brief Reserves capacity for at least the specified number of entities.
         *
         * @param n The number of entities to reserve capacity for.
         */
        void reserve(size_type n) {
            _entities.reserve(n);
            _values.reserve(n);
        }

    public:
        /** @brief Checks if the storage has no spawned entities. */
        [[nodiscard]] bool empty() const noexcept { return _count == 0; }

        /** @brief Returns the number of currently spawned (alive) entities. */
        [[nodiscard]] size_type size() const noexcept { return _count; }

        /** @brief Returns the total capacity of the underlying entity set. */
        [[nodiscard]] size_type capacity() const noexcept { return _entities.capacity(); }

        /** @brief Returns the component value at the specified index (const). */
        [[nodiscard]] const value_type& operator[](size_type index) const noexcept { return _values[index]; }

        /** @brief Returns the component value at the specified index (mutable). */
        [[nodiscard]] value_type& operator[](size_type index) noexcept { return _values[index]; }

    private:
        entities_type _entities;
        values_type _values;
        size_type _count = 0;
    };
} // namespace myth::storage