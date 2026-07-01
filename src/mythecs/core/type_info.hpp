#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <new>
#include <utility>
#include <cstring>

namespace myth::core {
    /**
     * @brief A structure that holds type information for a type, including its size, alignment, and functions for construction,
     * destruction, and swapping.
     */
    struct type_info final {
        /** @brief The type used for representing sizes and alignments. */
        using size_type = size_t;
        /** @brief The type used for representing the constructor function pointer. */
        using constructor_type = void(*)(void*, void*);
        /** @brief The type used for representing the destructor function pointer. */
        using destructor_type = void(*)(void*);
        /** @brief The type used for representing the swapper function pointer. */
        using swapper_type = void(*)(void*, void*);

        size_type _size;
        size_type _align;
        constructor_type _constructor;
        destructor_type _destructor;
        swapper_type _swapper;

        /**
         * @brief Constructs a type_info structure with the specified size, alignment, and function pointers for construction,
         * destruction, and swapping.
         * 
         * @param size The size of the type in bytes.
         * @param align The alignment of the type in bytes.
         * @param constructor A pointer to a function that can construct an object of the type in place, given a destination pointer
         * and a source pointer.
         * @param destructor A pointer to a function that can destruct an object of the type in place, given a pointer to the object.
         * @param swapper A pointer to a function that can swap two objects of the type in place, given pointers to the two objects.
         */
        constexpr type_info(size_type size, size_type align, constructor_type constructor, destructor_type destructor, swapper_type swapper) noexcept
            : _size(size), _align(align), _constructor(constructor), _destructor(destructor), _swapper(swapper) {}

        /** @brief Constructs a type_info object by copying another one. */
        constexpr type_info(const type_info&) = default;

        /** @brief Constructs a type_info object by moving another one. */
        constexpr type_info(type_info&&) noexcept = default;

        /** @brief Destroys the type_info object. */
        constexpr ~type_info() noexcept = default;

        /** @brief Assigns the contents of another type_info object to this one. */
        constexpr type_info& operator=(const type_info&) = default;

        /** @brief Moves the contents of another type_info object to this one. */
        constexpr type_info& operator=(type_info&&) noexcept = default;
    };

    namespace internal {
        /**
         * @brief A helper function for constructing objects of type T in place. For trivially copyable types, we can simply do
         * byte-to-byte copying. For non-trivially copyable types, we need to use the appropriate constructor to construct the object
         * in place. If the type is move constructible and not copy constructible, we can use the move constructor to construct the
         * object from the source, otherwise we fall back to the copy constructor.
         * 
         * @param dest A pointer to the memory location where the object should be constructed.
         * @param src A pointer to the source object from which to construct the new object.
         */
        template<typename T>
        inline void constructor_impl(void* dest, void* src) {
            if constexpr (std::is_trivially_copyable_v<T>) {
                std::memcpy(dest, src, sizeof(T));
            } else if constexpr (std::is_move_constructible_v<T> && !std::is_copy_constructible_v<T>) {
                new (dest) T(std::move(*static_cast<T*>(src)));
            } else {
                new (dest) T(*static_cast<T*>(src));
            }
        }

        /**
         * @brief A helper function for destructing objects of type T in place. For trivially destructible types, we don't need to do
         * anything to destruct the object. For non-trivially destructible types, we need to call the appropriate destructor to
         * destruct the object in place.
         * 
         * @param obj A pointer to the object to be destructed.
         */
        template<typename T>
        inline void destructor_impl(void* obj) {
            if constexpr (std::is_trivially_destructible_v<T>) {
                return;
            }

            static_cast<T*>(obj)->~T();
        }

        /**
         * @brief A helper function for swapping objects of type T in place. For trivially copyable types, we can do byte-to-byte
         * swapping using a temporary buffer. For non-trivially copyable types, we can use std::swap to swap the objects.
         * 
         * @param a A pointer to the first object to be swapped.
         * @param b A pointer to the second object to be swapped.
         */
        template<typename T>
        inline void swapper_impl(void* a, void* b) {
            if constexpr (std::is_trivially_copyable_v<T>) {
                std::byte temp[sizeof(T)];
                std::memcpy(temp, a, sizeof(T));
                std::memcpy(a, b, sizeof(T));
                std::memcpy(b, temp, sizeof(T));
            } else {
                std::swap(*static_cast<T*>(a), *static_cast<T*>(b));
            }
        }
    } // namespace internal

    /**
     * @brief A utility class that provides cached type information and unique sequential identifiers for types.
     *
     * `info<T>()` returns a const reference to a statically cached `type_info` record containing the size,
     * alignment, and lifecycle function pointers for `T`. `id<T>()` returns a dense sequential `uint32_t`
     * identifier assigned on first invocation - suitable for use as an array index in component pools.
     */
    class type_info_generator final {
    public:
        /** @brief The type of the generated type_info structure. */
        using info_type = type_info;
        /** @brief The type of the generated identifier. */
        using id_type = uint32_t;

        /**
         * @brief Returns a const reference to a statically cached type_info record for T.
         *
         * On first invocation, constructs and caches a type_info with the size, alignment, and
         * lifecycle function pointers (constructor / destructor / swapper) for T. Subsequent
         * calls return the same cached record.
         *
         * @tparam T The type for which to retrieve the type_info record.
         */
        template<typename T>
        inline static const info_type& info() noexcept {
            static info_type record(
                sizeof(T), alignof(T),
                &internal::constructor_impl<T>,
                &internal::destructor_impl<T>,
                &internal::swapper_impl<T>
            );

            return record;
        }

        /**
         * @brief Returns a dense sequential type identifier for T.
         *
         * On first invocation for a given T, assigns the next available `uint32_t` counter value
         * and caches it. Subsequent calls return the same cached identifier. Suitable for use as
         * a component-pool array index.
         *
         * @tparam T The type for which to retrieve the identifier.
         */
        template<typename T>
        inline static id_type id() noexcept {
            static id_type id = _cur++;

            return id;
        }

    private:
        inline static id_type _cur = 0;
    };
} // namespace myth::core