#pragma once

#include <cstddef>
#include <type_traits>
#include <cstdint>
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
        type_info(size_type size, size_type align, constructor_type constructor, destructor_type destructor, swapper_type swapper) noexcept
            : _size(size), _align(align), _constructor(constructor), _destructor(destructor), _swapper(swapper) {}

        /** @brief Constructs a type_info object by copying another one. */
        type_info(const type_info&) = default;

        /** @brief Constructs a type_info object by moving another one. */
        type_info(type_info&&) noexcept = default;

        /** @brief Destroys the type_info object. */
        ~type_info() = default;

        /** @brief Assigns the contents of another type_info object to this one. */
        type_info& operator=(const type_info&) = default;

        /** @brief Moves the contents of another type_info object to this one. */
        type_info& operator=(type_info&&) noexcept = default;
    };

    /**
     * @brief A utility class for generating type_info structures for specific types. This class provides a static member function
     * that can generate a type_info structure for any given type T, using the size and alignment of T, and providing appropriate
     * constructor, destructor, and swapper functions for managing objects of type T.
     */
    struct type_info_generator final {
        /** @brief The type of the generated type_info structure. */
        using info_type = type_info;

        /**
         * @brief Generates a type_info structure for the specified type T, and the result will be cached for subsequent calls with
         * the same type.
         * 
         * @tparam T The type for which to generate the type_info structure.
         */
        template<typename T>
        static const info_type& gen() noexcept {
            static info_type info(
                sizeof(T),
                alignof(T),
                [](void* dest, void* src) {
                    // For trivially copyable types, we can simply use memcpy to copy the object. For non-trivially copyable
                    // types, we need to use the appropriate constructor to construct the object in place. If the type is move
                    // constructible and not copy constructible, we can use the move constructor to construct the object from
                    // the source, otherwise we fall back to the copy constructor.
                    if constexpr (std::is_trivially_copyable_v<T>) {
                        std::memcpy(dest, src, sizeof(T));
                    } else if constexpr (std::is_move_constructible_v<T> && !std::is_copy_constructible_v<T>) {
                        new (dest) T(std::move(*static_cast<T*>(src)));
                    } else {
                        new (dest) T(*static_cast<T*>(src));
                    }
                },
                [](void* obj) {
                    // For trivially destructible types, we don't need to do anything to destruct the object. For non-trivially
                    // destructible types, we need to call the appropriate destructor to destruct the object in place.
                    if constexpr (std::is_trivially_destructible_v<T>) {
                        return;
                    }

                    static_cast<T*>(obj)->~T();
                },
                [](void* a, void* b) {
                    // For trivially copyable types, we can use memcpy to swap the objects using a temporary buffer. For
                    // non-trivially copyable types, we can use std::swap to swap the objects.
                    if constexpr (std::is_trivially_copyable_v<T>) {
                        uint8_t temp[sizeof(T)];
                        std::memcpy(&temp, a, sizeof(T));
                        std::memcpy(a, b, sizeof(T));
                        std::memcpy(b, &temp, sizeof(T));
                    } else {
                        std::swap(*static_cast<T*>(a), *static_cast<T*>(b));
                    }
                }
            );

            return info;
        }
    };
} // namespace myth::core