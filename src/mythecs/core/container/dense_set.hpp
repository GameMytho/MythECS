#pragma once

#include <bit>
#include <cmath>
#include <vector>
#include <core/compressed_pair.hpp>

namespace myth::core::container {
    /**
     * @brief An open-addressing hash set using a split dense/sparse layout.
     *
     * The dense set stores its elements in two parallel structures. The density array is a vector of nodes, where each node
     * holds a key together with the index of the next node in the same bucket chain. The sparsity array is a vector of bucket
     * heads, each of which is either an index into the density array or the sentinel `null_key_index` for an empty bucket.
     *
     * Hash and key-equality functors are stored via `compressed_pair` to take advantage of Empty Base Class Optimization (EBCO)
     * when the functors are stateless.
     *
     * @tparam KeyType   The type of keys stored in the set.
     * @tparam Hash      A unary functor (default: `std::hash`) that computes the hash of a key.
     * @tparam KeyEqual  A binary functor (default: `std::equal_to`) that compares two keys for equality.
     * @tparam Allocator The allocator type used for internal containers (default: `std::allocator`).
     */
    template<
        typename KeyType,
        template<typename> typename Hash = std::hash,
        template<typename> typename KeyEqual = std::equal_to,
        template<typename> typename Allocator = std::allocator
    >
    class dense_set final {
    public:
        /** @brief The type of keys stored in the set. */
        using key_type = KeyType;
        /** @brief The hash functor type. */
        using hasher_type = Hash<key_type>;
        /** @brief The key-equality functor type. */
        using keyeq_type = KeyEqual<key_type>;
        /** @brief The allocator type. */
        using allocator_type = Allocator<key_type>;
        /** @brief The unsigned integral type used for sizes and indices. */
        using size_type = size_t;
        /** @brief A node in the density array: a pair of (next_index, key). */
        using node_type = std::pair<size_type, key_type>;
        /** @brief The type of the density array (a vector of nodes). */
        using nodes_type = std::vector<node_type, Allocator<node_type>>;
        /** @brief The density container: a compressed_pair of (nodes vector, key-equality functor). */
        using density_type = myth::core::compressed_pair<nodes_type, keyeq_type>;
        /** @brief The type of the sparsity array (a vector of bucket head indices). */
        using buckets_type = std::vector<size_type, Allocator<size_type>>;
        /** @brief The sparsity container: a compressed_pair of (buckets vector, hash functor). */
        using sparsity_type = myth::core::compressed_pair<buckets_type, hasher_type>;

        /** @brief The default load-factor threshold that triggers a rehash. */
        inline static constexpr float default_threshold = 0.875f;
        /** @brief The minimum number of buckets (must be a power of two). */
        inline static constexpr size_type minimum_bucket_count = 8u;
        /** @brief A sentinel value indicating an invalid/empty bucket entry. */
        inline static constexpr size_type null_key_index = std::numeric_limits<size_type>::max();

        /**
         * @brief Constructs an empty dense set with the default threshold and minimum bucket count.
         *
         * Initializes `_threshold` to `default_threshold` and allocates `minimum_bucket_count` buckets.
         */
        dense_set() : _threshold(default_threshold) {
            rehash(0u);
        }

        /**
         * @brief Constructs an empty dense set with a specified element capacity and custom allocator.
         *
         * Uses default-constructed hash and key-equality functors. Delegates to the full constructor,
         * which calls `reserve(capacity)` to pre-allocate both the density array and the bucket array.
         *
         * @param capacity  The minimum number of elements to reserve capacity for.
         * @param allocator The allocator for internal containers.
         */
        dense_set(size_type capacity, const allocator_type& allocator = allocator_type{}) noexcept
            : dense_set{capacity, hasher_type{}, keyeq_type{}, allocator} {}

        /**
         * @brief Constructs an empty dense set with a specified element capacity, custom hash functor, and custom allocator.
         *
         * Uses a default-constructed key-equality functor. Delegates to the full constructor.
         *
         * @param capacity  The minimum number of elements to reserve capacity for.
         * @param hasher    The hash functor to use.
         * @param allocator The allocator for internal containers.
         */
        dense_set(size_type capacity, const hasher_type& hasher, const allocator_type& allocator) noexcept
            : dense_set{capacity, hasher, keyeq_type{}, allocator} {}

        /**
         * @brief Constructs an empty dense set with a specified element capacity, custom hash
         *        functor, custom key-equality functor, and custom allocator.
         *
         * Initializes the threshold and both compressed pairs, then calls `reserve(capacity)`
         * to pre-allocate the density array and size the bucket array. See `reserve()` for
         * details on how the bucket count is determined.
         *
         * @param capacity  The minimum number of elements to reserve capacity for.
         * @param hasher    The hash functor to use.
         * @param keyeq     The key-equality functor to use.
         * @param allocator The allocator for internal containers.
         */
        dense_set(size_type capacity, const hasher_type& hasher, const keyeq_type& keyeq, const allocator_type& allocator) noexcept
            : _threshold{default_threshold}, _sparsity{allocator, hasher}, _density{allocator, keyeq} {
            reserve(capacity);
        }

        /** @brief Copy constructor. */
        dense_set(const dense_set&) = default;

        /**
         * @brief Allocator-extended copy constructor.
         *
         * Copies the elements and functors from `other` while using `allocator` for the internal containers.
         *
         * @param other     The dense set to copy from.
         * @param allocator The allocator to use for the new containers.
         */
        dense_set(const dense_set& other, const allocator_type& allocator)
            : _sparsity{std::piecewise_construct, std::forward_as_tuple(other._sparsity.first(), allocator), std::forward_as_tuple(other._sparsity.second())},
            _density{std::piecewise_construct, std::forward_as_tuple(other._density.first(), allocator), std::forward_as_tuple(other._density.second())},
            _threshold{other._threshold} {}

        /** @brief Move constructor. */
        dense_set(dense_set&& other) noexcept = default;

        /**
         * @brief Allocator-extended move constructor.
         *
         * Moves the elements and functors from `other` while using `allocator` for the internal containers.
         *
         * @param other     The dense set to move from.
         * @param allocator The allocator to use for the new containers.
         */
        dense_set(dense_set&& other, const allocator_type& allocator) noexcept
            : _density{std::piecewise_construct, std::forward_as_tuple(std::move(other._density.first()), allocator), std::forward_as_tuple(std::move(other._density.second()))},
            _sparsity{std::piecewise_construct, std::forward_as_tuple(std::move(other._sparsity.first()), allocator), std::forward_as_tuple(std::move(other._sparsity.second()))},
            _threshold{other._threshold} {}

        /** @brief Destructor. */
        ~dense_set() noexcept = default;

        /** @brief Copy assignment operator. */
        dense_set& operator=(const dense_set&) = default;

        /** @brief Move assignment operator. */
        dense_set& operator=(dense_set&&) noexcept = default;

    public:
        /**
         * @brief Inserts a key into the set. If the key already exists, this is a no-op.
         *
         * If the sparsity array is empty (i.e. the set is in a moved-from state), `rehash(0u)`
         * is called first to initialize the bucket array. Hashes the key to locate its bucket,
         * walks the bucket's chain to check for duplicates, appends a new node to the density
         * array (with the current bucket head as its next link), sets the bucket head to the
         * new node, and triggers a rehash if the load factor exceeds `_threshold`.
         *
         * @param key The key to insert.
         */
        void emplace(const key_type& key) {
            [[unlikely]] if (_sparsity.first().empty()) {
                rehash(0u);
            }

            size_type index = key_to_bucket(key);

            if (find_index(key, index) != null_key_index) {
                return;
            }

            _density.first().emplace_back(_sparsity.first()[index], key);
            _sparsity.first()[index] = size() - 1;
            rehash_if_required();
        }

        /**
         * @brief Removes a key from the set. If the key does not exist, this is a no-op.
         *
         * Returns immediately if the set is empty. Otherwise, walks the bucket chain to locate
         * the node containing the key, unlinks it from the chain, then calls `move_and_pop()` to
         * compact the density array by back-filling the hole with the last element and patching
         * the chain pointer that referenced it.
         *
         * @param key The key to remove.
         */
        void erase(const key_type& key) noexcept {
            [[unlikely]] if (empty()) {
                return;
            }

            size_type index = key_to_bucket(key);
            size_type* cur = &_sparsity.first()[index];
            for (; *cur != null_key_index; cur = &_density.first()[*cur].first) {
                if (_density.second()(_density.first()[*cur].second, key)) {
                    const size_type idx = *cur;
                    *cur = _density.first()[*cur].first;
                    move_and_pop(idx);
                    break;
                }
            }
        }

        /**
         * @brief Swaps two elements in the set by their density indices.
         *
         * Locates the chain predecessor of each element (the bucket head or prior node whose `first()`
         * points to it), patches the two predecessor pointers to reference each other's position, then
         * swaps the nodes in the density array. This keeps the bucket chains intact while the nodes
         * exchange places.
         *
         * @param lh The density index of the first element.
         * @param rh The density index of the second element.
         *
         * @warning The behavior is undefined if `lh` or `rh` is out of range.
         */
        void swap(size_type lh, size_type rh) {
            const key_type& lk = _density.first()[lh].second;
            size_type* lp = &_sparsity.first()[key_to_bucket(lk)];
            for (; *lp != lh; lp = &_density.first()[*lp].first) {}

            const key_type& rk = _density.first()[rh].second;
            size_type* rp = &_sparsity.first()[key_to_bucket(rk)];
            for (; *rp != rh; rp = &_density.first()[*rp].first) {}

            *lp = rh;
            *rp = lh;

            std::swap(_density.first()[lh], _density.first()[rh]);
        }

        /**
         * @brief Returns the density index of a key, or `null_key_index` if the key is not found.
         *
         * Returns `null_key_index` immediately if the set is empty. Otherwise, hashes the key and
         * walks the bucket chain to locate a matching node.
         *
         * @param key The key to search for.
         * @return The density index of the key, or `null_key_index` if not found.
         */
        [[nodiscard]] size_type index(const key_type& key) const noexcept {
            [[unlikely]] if (empty()) {
                return null_key_index;
            }

            return find_index(key, key_to_bucket(key));
        }

        /**
         * @brief Checks whether a key exists in the set.
         *
         * Returns `false` immediately if the set is empty. Otherwise, hashes the key and
         * walks the bucket chain to locate a matching node.
         *
         * @param key The key to check for existence.
         * @return `true` if the key exists in the set, `false` otherwise.
         */
        [[nodiscard]] bool contains(const key_type& key) const noexcept {
            [[unlikely]] if (empty()) {
                return false;
            }

            return find_index(key, key_to_bucket(key)) != null_key_index;
        }

        /**
         * @brief Clears all elements from the set and rehashes to the minimum bucket count.
         *
         * The density and sparsity arrays are cleared, then `rehash(0u)` is called to restore
         * `minimum_bucket_count` buckets. The load-factor threshold is preserved.
         */
        void clear() {
            _density.first().clear();
            _sparsity.first().clear();
            rehash(0u);
        }

        /**
         * @brief Shrinks the internal storage to fit the current number of elements.
         *
         * Rehashes the bucket array to the minimum size needed for the current elements
         * (at least `minimum_bucket_count`) and requests the density array to release any
         * excess capacity.
         */
        void shrink_to_fit() {
            rehash(0u);
            _density.first().shrink_to_fit();
        }

        /**
         * @brief Reserves capacity for at least the specified number of elements.
         *
         * Pre-allocates the density array with `n` elements of storage and resizes the bucket array
         * to hold at least `ceil(n / max_load_factor())` entries (rounded up to the next power of two).
         * After a successful `reserve(n)`, inserting up to `n` elements will not cause a reallocation
         * of the density array.
         *
         * @param n The minimum number of elements to reserve capacity for.
         */
        void reserve(size_type n) {
            _density.first().reserve(n);
            rehash(static_cast<size_type>(std::ceil(static_cast<float>(n) / max_load_factor())));
        }

        /**
         * @brief Sets the maximum load factor and rehashes the set.
         *
         * If the new threshold is lower than the current load factor, the bucket count
         * is increased to maintain the invariant. Otherwise, the bucket count is reduced
         * to the minimum needed for the current elements.
         *
         * @param value The new load factor threshold.
         *
         * @warning `value` must be greater than zero; a non-positive value results in
         *          undefined behavior due to division by zero in internal calculations.
         */
        void max_load_factor(const float value) noexcept {
            _threshold = value;
            rehash(0u);
        }

    public:
        /** @brief Checks whether the set is empty. */
        [[nodiscard]] bool empty() const noexcept { return _density.first().empty(); }

        /** @brief Returns the number of elements in the set. */
        [[nodiscard]] size_type size() const noexcept { return _density.first().size(); }

        /** @brief Returns the capacity of the density array. */
        [[nodiscard]] size_type capacity() const noexcept { return _density.first().capacity(); }

        /** @brief Accesses the key at the specified density index (mutable). */
        [[nodiscard]] key_type& operator[](size_type idx) noexcept { return _density.first()[idx].second; }

        /** @brief Accesses the key at the specified density index (const). */
        [[nodiscard]] const key_type& operator[](size_type idx) const noexcept { return _density.first()[idx].second; }

        /** @brief Returns the number of buckets in the sparsity array. */
        [[nodiscard]] const size_type bucket_count() const noexcept { return _sparsity.first().size(); }

        /** @brief Returns the current load factor (`size() / bucket_count()`). */
        [[nodiscard]] const float load_factor() const noexcept { return static_cast<float>(size()) / static_cast<float>(bucket_count()); }

        /** @brief Returns the current max load factor threshold. */
        [[nodiscard]] const float max_load_factor() const noexcept { return _threshold; }

    private:
        float _threshold;
        sparsity_type _sparsity;
        density_type _density;

    private:
        /**
         * @brief Triggers a rehash if the load factor exceeds the threshold.
         *
         * When the number of elements exceeds `_threshold * bucket_count()`, requests
         * `size() * 2` buckets via `rehash()` to restore the load-factor invariant.
         */
        void rehash_if_required() {
            if (size_type s = size(); s > static_cast<size_type>(max_load_factor() * bucket_count())) {
                rehash(s * 2u);
            }
        }

        /**
         * @brief Back-fills the hole at `index` with the last element in the density array, then pops the last slot.
         *
         * If `index` is not the last position, the last element is moved into `index` and the bucket chain
         * pointer that previously targeted the last element's old position (`last_idx`) is patched to point
         * to `index` instead. If `index` is already the last position, only `pop_back()` is performed.
         *
         * @param index The density index of the element to remove.
         */
        void move_and_pop(size_type index) noexcept {
            if (size_type last_idx = size() - 1; index != last_idx) {
                size_type* cur = &_sparsity.first()[key_to_bucket(_density.first().back().second)];
                _density.first()[index] = std::move(_density.first().back());
                for (; *cur != last_idx; cur = &_density.first()[*cur].first) {}
                *cur = index;
            }

            _density.first().pop_back();
        }

        /**
         * @brief Resizes the bucket array to the specified capacity and re-inserts all elements.
         *
         * The actual bucket count is rounded up to the next power of two (at least `minimum_bucket_count`
         * and at least large enough to hold all current elements under the load threshold). If the
         * resulting size is unchanged, this is a no-op.
         *
         * @param n The requested new bucket count (may be adjusted upward).
         */
        void rehash(size_type n) {
            size_type new_cap = n > minimum_bucket_count ? n : minimum_bucket_count;
            size_type needed_cap = static_cast<size_type>(static_cast<float>(size()) / max_load_factor());
            new_cap = new_cap > needed_cap ? new_cap : needed_cap;

            size_type old_cap = bucket_count();
            if (size_type new_size = std::bit_ceil(new_cap); new_size != old_cap) {
                std::fill(_sparsity.first().begin(), _sparsity.first().end(), null_key_index);
                _sparsity.first().resize(new_size, null_key_index);

                size_type n = size();
                for (size_type i = 0; i < n; ++i) {
                    size_type index = key_to_bucket(_density.first()[i].second);
                    _density.first()[i].first = std::exchange(_sparsity.first()[index], i);
                }
            }
        }

        /**
         * @brief Computes the bucket index for a given key.
         *
         * Hashes the key and reduces the result modulo the current bucket count.
         *
         * @param key The key to hash.
         * @return The bucket index (in `[0, bucket_count)`).
         * 
         * @note The bucket count is always a power of two (guaranteed by `std::bit_ceil` in `rehash`),
         * so the modulo reduction is implemented as a fast bitwise AND with `(size - 1)`.
         */
        [[nodiscard]] size_type key_to_bucket(const key_type& key) const noexcept {
            return _sparsity.second()(key) & (bucket_count() - 1);
        }

        /**
         * @brief Finds the density index of a key given its bucket index.
         *
         * Walks the chain starting at the given bucket and returns the index of the node whose key
         * compares equal, or `null_key_index` if not found.
         *
         * @param key    The key to search for.
         * @param bucket The bucket index (pre-computed from `key_to_bucket()`).
         * @return The density index of the key, or `null_key_index` if not found.
         */
        [[nodiscard]] size_type find_index(const key_type& key, size_type bucket) const noexcept {
            const size_type* cur = &_sparsity.first()[bucket];
            for (; *cur != null_key_index; cur = &_density.first()[*cur].first) {
                if (_density.second()(_density.first()[*cur].second, key)) {
                    return *cur;
                }
            }

            return null_key_index;
        }
    };
}