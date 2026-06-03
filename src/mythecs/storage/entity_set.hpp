#pragma once

#include <cstddef>
#include <vector>
#include <memory>

#include "core/container/sparse_set.hpp"

namespace myth::storage {
    /**
     * @brief A set of entities with associated IDs and versions.
     *
     * This class manages a collection of entities, where each entity is identified by a unique ID and has an associated version.
     * It uses a sparse set to efficiently store and manage the entity IDs, and a vector to store the corresponding versions.
     * The sparse set allows for fast insertion, deletion, and lookup of entity IDs, while the vector provides efficient storage 
     * for the versions.
     * 
     * @tparam EntityType The type of the entities stored in the set, which must have an entity_id_type and entity_version_type defined.
     * @tparam Allocator The allocator type used for memory management of the underlying data structures. Defaults to std::allocator.
     * @tparam CacheLineSize The size of a cache line in bytes, used for cache-line alignment in the sparse set.
     * @tparam PageSize The number of entries in each page of the sparse set.
     */
    template<
        typename EntityType,
        template<typename> typename Allocator = std::allocator,
        size_t CacheLineSize = 64,
        size_t PageSize = 256
    >
    class entity_set final {
        /** @brief Friend class declaration for entity_storage. */
        template<typename, typename, template<typename> typename, size_t, size_t>
        friend class entity_storage;

    public:
        /** @brief The type of the entities stored in the set. */
        using entity_type = EntityType;
        /** @brief The type of the entity IDs. */
        using entity_id_type = typename entity_type::entity_id_type;
        /** @brief The type of the set of entity IDs. */
        using entity_ids_type = ::myth::core::container::sparse_set<entity_id_type, Allocator, CacheLineSize, PageSize>;
        /** @brief The type used for sizes and indices in the entity set. */
        using size_type = typename entity_ids_type::size_type;
        /** @brief The type used for indices of entities in the set. */
        using entity_index_type = typename entity_ids_type::value_index_type;
        /** @brief The type of the entity versions. */
        using entity_version_type = typename entity_type::entity_version_type;
        /** @brief The type of the vector storing entity versions. */
        using entity_versions_type = std::vector<entity_version_type, Allocator<entity_version_type>>;

        /** @brief A special value indicating an invalid index in the entity set. */
        inline static constexpr entity_index_type null_entity_index = entity_ids_type::null_value_index;

        /** @brief Constructs an empty entity set. */
        entity_set() noexcept = default;

        /**
         * @brief Constructs an entity set with the specified initial capacity for the entity IDs and versions.
         * 
         * @param n The initial capacity for both the entity IDs and versions.
         */
        entity_set(size_type n) {
            _ids.reserve(n);
            _versions.reserve(n);
        }

        /** @brief Constructs an entity set by copying another one. */
        entity_set(const entity_set&) = default;

        /** @brief Constructs an entity set by moving another one. */
        entity_set(entity_set&&) noexcept = default;

        /** @brief Destructs the entity set. */
        ~entity_set() noexcept = default;

        /** @brief Assigns another entity set to this one. */
        entity_set& operator=(const entity_set&) = default;

        /** @brief Moves another entity set to this one. */
        entity_set& operator=(entity_set&&) noexcept = default;

    public:
        /**
         * @brief Emplaces an entity into the set.
         *
         * @param entt The entity to emplace.
         * @return The index of the emplaced entity.
         * 
         * @warning Before calling this function, ensure that the entity does not already exist in the set by
         * using the contains() function, otherwise the behavior is undefined.
         */
        size_type emplace(const entity_type& entt) {
            _versions.push_back(entt.version());
            return _ids.emplace(entt.id());
        }

        /**
         * @brief Erases an entity from the set.
         *
         * @param entt The entity to erase.
         * 
         * @warning Before calling this function, ensure that the entity exists in the set by using the
         * contains() function, otherwise the behavior is undefined.
         */
        void erase(const entity_type& entt) noexcept {
            entity_id_type id = entt.id();
            size_type index = _ids.index(id);

            _ids.erase(id);

            if (index != _versions.size() - 1) {
                _versions[index] = _versions.back();
            }
            _versions.pop_back();
        }

        /**
         * @brief Gets the index of an entity in the set by its ID.
         * 
         * @param entt The entity to find.
         * @return The index of the entity if found, or null_entity_index if not found.
         */
        [[nodiscard]] entity_index_type index(const entity_type& entt) const noexcept {
            size_type index = _ids.index(entt.id());
            if (index < _versions.size() && _versions[index] == entt.version()) {
                return index;
            }

            return null_entity_index;
        }

        /**
         * @brief Checks if an entity exists in the set.
         * 
         * @param entt The entity to check.
         * @return True if the entity exists, false otherwise.
         */
        [[nodiscard]] bool contains(const entity_type& entt) const noexcept {
            size_type index = _ids.index(entt.id());
            return index != null_entity_index && index < _versions.size() && _versions[index] == entt.version();
        }

        /**
         * @brief Clears the set, removing all entities.
         */
        void clear() noexcept {
            _ids.clear();
            _versions.clear();
        }

        /**
         * @brief Shrinks the capacity of the set to fit its size.
         */
        void shrink_to_fit() {
            _ids.shrink_to_fit();
            _versions.shrink_to_fit();
        }

        /**
         * @brief Reserves capacity for at least the specified number of entities.
         * 
         * @param n The number of entities to reserve capacity for.
         */
        void reserve(size_type n) {
            _ids.reserve(n);
            _versions.reserve(n);
        }

    public:
        /** @brief Checks if the set is empty. */
        [[nodiscard]] bool empty() const noexcept { return _ids.empty(); }

        /** @brief Returns the number of entities in the set. */
        [[nodiscard]] size_type size() const noexcept { return _ids.size(); }

        /** @brief Returns the capacity of the set. */
        [[nodiscard]] size_type capacity() const noexcept { return _ids.capacity(); }

        /** @brief Returns the entity at the specified index. */
        [[nodiscard]] const entity_type operator[](size_type index) const noexcept {
            return entity_type(_ids[index], _versions[index]);
        }

    private:
        entity_ids_type _ids;
        entity_versions_type _versions;

    private:
        /**
         * @brief Swaps two entities in the set by their indices.
         * 
         * @param lh The index of the first entity to swap.
         * @param rh The index of the second entity to swap.
         */
        void swap(size_type lh, size_type rh) noexcept {
            _ids.swap(lh, rh);
            std::swap(_versions[lh], _versions[rh]);
        }

        /**
         * @brief Increments the version of the entity at the specified index.
         * 
         * @param index The index of the entity whose version to increment.
         */
        void version_next(size_type index) noexcept {
            ++_versions[index];
        }
    };
} // namespace myth::storage