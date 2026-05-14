#pragma once

#include <vector>
#include <array>
#include <memory>
#include <limits>

#include "core/concept.hpp"

/*
 * @brief A sparse set implementation based on std::vector.
 * 
 * The sparse set is a data structure that provides efficient storage and retrieval of values based on their indices.
 * It consists of a density vector that stores the actual values and a sparsity vector that maps the indices to the positions
 * in the density vector. The sparsity vector is organized into pages, each containing a fixed number of entries, and is
 * aligned to cache line size for better performance.
 * 
 * @tparam Type The unsigned integral value type stored in the sparse set.
 * @tparam Allocator The allocator type used for memory management of the density and sparsity vectors. Defaults to std::allocator.
 * @tparam CacheLineSize The size of a cache line in bytes. Defaults to 64 bytes.
 * @tparam PageSize The number of entries in each page of the sparsity vector. Defaults to 256 entries.
 */
namespace myth::core::container {
    template<
        ::myth::core::UnsignedIntegralType Type,
        template<typename> typename Allocator = std::allocator,
        size_t CacheLineSize = 64,
        size_t PageSize = 256
    >
    class sparse_set final {
    public:
        /*! @brief The value type stored in the density array. */
        using value_type                    = Type;
        /*! @brief The density vector type. */
        using density_type                  = std::vector<value_type, Allocator<value_type>>;
        /*! @brief The size type. */
        using size_type                     = typename density_type::size_type;
        /*! @brief The difference type for pointer arithmetic. */
        using difference_type               = typename density_type::difference_type;
        /*! @brief The value index type in the density vector, stored in the sparsity vector. */
        using value_index_type              = size_type;
        /*! @brief The type of each page in the sparsity vector, stored as an array aligned to CacheLineSize. */
        struct alignas(CacheLineSize) page_type : public std::array<value_index_type, PageSize> {};
        /*! @brief The type of the sparsity vector. */
        using sparsity_type                 = std::vector<page_type, Allocator<page_type>>;
        /*! @brief Constant random access iterator type. */
        using const_iterator_type           = typename density_type::const_iterator;
        /*! @brief Constant reverse random access iterator type. */
        using const_reverse_iterator_type   = typename density_type::const_reverse_iterator;

        /*! @brief A special(placeholder) value indicating an invalid index in the sparsity vector. */
        static constexpr value_index_type null_value_index = std::numeric_limits<value_index_type>::max();

        /*! @brief Constructs an empty sparse set. */
        sparse_set() noexcept = default;

        /*! @brief Constructs a sparse set with the specified initial capacities for the density and sparsity vectors. */
        sparse_set(size_t density_capacity, size_t sparsity_page_count = 0) {
            _density.reserve(density_capacity);
            _sparsity.reserve(sparsity_page_count);
        }

        /*! @brief Constructs a sparse set by copying another one. */
        sparse_set(const sparse_set&) = default;

        /*! @brief Constructs a sparse set by moving another one. */
        sparse_set(sparse_set&&) noexcept = default;

        /*! @brief Destructs the sparse set. */
        ~sparse_set() = default;

        /*! @brief Assigns the contents of another sparse set to this one. */
        sparse_set& operator=(const sparse_set&) = default;

        /*! @brief Moves the contents of another sparse set to this one. */
        sparse_set& operator=(sparse_set&&) noexcept = default;

    public:
        /*
         * @brief Inserts a value into the sparse set and returns its index.
         * @warning before calling this function, make sure the value does not exist in the sparse set by calling contains() function,
         * otherwise the behavior is undefined.
         * 
         * Add the value to the end of the density vector, and the sparsity vector is updated to map the value's index
         * to its position in the density vector.
         * 
         * @param value The value to be inserted into the density vector.
         * @return The index of the inserted value in the density vector.
         */
        size_t emplace(value_type value) {
            const value_index_type value_index = _density.size();

            _density.push_back(value);
            expand(page(value))[offset(value)] = value_index;

            return value_index;
        }

        /*
         * @brief Removes a value from the sparse set.
         * @warning before calling this function, make sure the value exists in the sparse set by calling contains() function,
         * otherwise the behavior is undefined.
         * 
         * Replaces the value with the last value in the density vector and updates the sparse vector accordingly,
         * and then removes the last element from the density vector.
         * 
         * @param value The value to be removed from the sparse set.
         */
        void erase(value_type value) noexcept {
            value_type last_value = _density.back();
            value_index_type& value_index = _sparsity[page(value)][offset(value)];

            if (value != last_value) {
                _sparsity[page(last_value)][offset(last_value)] = value_index;
                _density[value_index] = last_value;
            }

            value_index = null_value_index;
            _density.pop_back();
        }

        /*
         * @brief Returns the index of a value in the sparse set.
         * @warning before calling this function, make sure the value exists in the sparse set by calling contains() function,
         * otherwise the behavior will trigger access out of bounds and cause undefined behavior.
         *
         * @param value The value to search for.
         * @return The index of the value in the density vector, or null_value_index if not found.
         */
        [[nodiscard]] size_t index(value_type value) noexcept {
            return _sparsity[page(value)][offset(value)];
        }

        /*
         * @brief Checks if a value exists in the sparse set.
         *
         * @param value The value to check for existence.
         * @return true if the value exists in the sparse set, false otherwise.
         */
        [[nodiscard]] bool contains(value_type value) noexcept {
            size_t index = page(value);
            if (index >= _sparsity.size()) {
                return false;
            }

            return _sparsity[index][offset(value)] != null_value_index;
        }

        /*
         * @brief Clears all values from the sparse set, leaving it empty.
        */
        void clear() noexcept {
            _density.clear();
            _sparsity.clear();
        }

        /*
         * @brief Resizes the sparse set to fit its contents.
         */
        void shrink_to_fit() {
            _density.shrink_to_fit();
            _sparsity.shrink_to_fit();
        }

        /*
         * @brief Reserves storage for at least the specified number of elements in the density vector.
         */
        void reserve(size_t new_capacity) {
            _density.reserve(new_capacity);
        }

    public:
        /*! @brief Returns a constant iterator to the beginning of the density vector. */
        [[nodiscard]] const_iterator_type cbegin() const noexcept { return _density.cbegin(); }

        /*! @brief Returns a constant iterator to the end of the density vector. */
        [[nodiscard]] const_iterator_type cend() const noexcept { return _density.cend(); }

        /*! @brief Returns a constant reverse iterator to the beginning of the density vector. */
        [[nodiscard]] const_reverse_iterator_type crbegin() const noexcept { return _density.crbegin(); }

        /*! @brief Returns a constant reverse iterator to the end of the density vector. */
        [[nodiscard]] const_reverse_iterator_type crend() const noexcept { return _density.crend(); }

        /*! @brief Checks if the sparse set is empty. */
        [[nodiscard]] bool empty() const noexcept { return _density.empty(); }

        /*! @brief Returns the number of elements in the sparse set. */
        [[nodiscard]] size_t size() const noexcept { return _density.size(); }

        /*! @brief Returns the capacity of the density vector. */
        [[nodiscard]] size_t capacity() const noexcept { return _density.capacity(); }

        /*! @brief Returns the maximum number of elements the sparse set can hold. */
        [[nodiscard]] size_t max_size() const noexcept { return _density.max_size(); }

        /*! @brief Returns the number of pages in the sparsity vector. */
        [[nodiscard]] size_t page_count() const noexcept { return _sparsity.size(); }

        /*! @brief Returns the capacity of the sparsity vector. */
        [[nodiscard]] size_t page_capacity() const noexcept { return _sparsity.capacity(); }

        /*! @brief Provides read-only access to the value at the specified index in the density vector. */
        [[nodiscard]] const value_type& operator[](size_t index) const noexcept { return _density[index]; }

    private:
        density_type _density;
        sparsity_type _sparsity;

    private:
        /*
         * @brief Calculates the page index for a given value.
         *
         * @param value The value for which to calculate the page index.
         * @return The page index of the value.
         */
        size_t page(value_type value) {
            return value / PageSize;
        }

        /*
         * @brief Calculates the offset within a page for a given value.
         *
         * @param value The value for which to calculate the offset.
         * @return The offset of the value within its page.
         */
        size_t offset(value_type value) {
            return value % PageSize;
        }

        /*
         * @brief Expands the sparsity vector to accommodate a new index if capacity is insufficient, otherwise returns the existing page.
         *
         * @param index The page index to expand to.
         * @return A reference to the page in the sparsity vector corresponding to the given index.
         */
        page_type& expand(size_t index) {
            size_t old_size = _sparsity.size();
            size_t new_size = index + 1;
            if (new_size > old_size) {
                page_type new_page;
                new_page.fill(null_value_index);
                _sparsity.resize(new_size, new_page);
            }

            return _sparsity[index];
        }
    };
} // namespace myth::container