#pragma once

#include <memory>
#include <cstddef>
#include <vector>
#include <cstdint>
#include <utility>

#include "core/type_info.hpp"

namespace myth::core::container {
    /**
     * @brief A type-erased vector-like container that can store elements of any type, as long as the type information
     * is provided.
     * 
     * This container uses a type_info structure to manage the construction, destruction, and swapping of elements, allowing
     * it to store any type of element without knowing the type at compile time. The container manages its own memory and
     * provides functionalities similar to a standard vector, such as emplace_back, pop_back, swap, clear, shrink_to_fit, and
     * reserve. The elements are stored in a contiguous buffer, and the container ensures proper alignment and memory
     * management for the stored elements.
     * 
     * @tparam Allocator The allocator type used for memory management of the container. Defaults to std::allocator.
     */
    template<template<typename> typename Allocator = std::allocator>
    class any_vector final {
    public:
        /** @brief The type of the buffer pointer. */
        using buffer_type = uint8_t*;
        /** @brief The type used to represent the size of the container. */
        using size_type = size_t;
        /** @brief The type used to store type information. */
        using info_type = type_info;

        /**
         * @brief Constructs an empty any_vector with the specified type information.
         */
        any_vector(const info_type& info) noexcept
            : _buffer(nullptr), _size(0), _capacity(0), _info(info) {}

        /**
         * @brief Constructs an any_vector by copying another one. This operation is deleted to prevent copying of the
         * container, as it manages its own memory and copying would require deep copying of the elements, which is not
         * supported by this container.
         */
        any_vector(const any_vector&) = delete;

        /**
         * @brief Constructs an any_vector by moving another one. This operation transfers ownership of the buffer and type
         * information from the source any_vector to the new one, leaving the source any_vector in a valid but empty state.
         * 
         * @param vec The any_vector to move from.
         */
        any_vector(any_vector&& vec) noexcept
            : _buffer(std::exchange(vec._buffer, nullptr)), _size(std::exchange(vec._size, 0)),
            _capacity(std::exchange(vec._capacity, 0)), _info(vec._info) {}

        /**
         * @brief Destroys the any_vector, properly destructing all elements and deallocating the buffer. If the buffer
         * is not null, it will call clear() to destruct all elements and then deallocate the buffer using the appropriate
         * alignment specified in the type information. If the buffer is null, it simply returns without doing anything.
         */
        ~any_vector() noexcept {
            if(!_buffer) {
                return;
            }

            clear();

            ::operator delete(_buffer, std::align_val_t(_info._align));
        }

        /**
         * @brief Assigns another any_vector to this one. This operation is deleted to prevent copying of the container, as
         * it manages its own memory and copying would require deep copying of the elements, which is not supported by this
         * container.
         */
        any_vector& operator=(const any_vector&) = delete;

        /**
         * @brief Moves another any_vector to this one. This operation transfers ownership of the buffer and type information 
         * from the source any_vector to this one, properly destructing any existing elements and deallocating the existing buffer
         * before taking ownership of the new buffer and type information. After the move, the source any_vector is left in a valid
         * but empty state.
         * 
         * @param vec The any_vector to move from.
         * @return A reference to this any_vector after the move.
         */
        any_vector& operator=(any_vector&& vec) noexcept = delete;

    public:
        /**
         * @brief Adds a new element to the end of the container by constructing it in place using the provided source
         * pointer. If the current size of the container is equal to its capacity, it will first reserve more space
         * (doubling the capacity) before constructing the new element. The source pointer is expected to point to an object
         * of the type described by the type information, and the constructor function in the type information will be used
         * to construct the new element in place.
         * 
         * @param src A pointer to the source object from which to construct the new element. The type of the object pointed
         * to by src should match the type described by the type information of the container.
         */
        void emplace_back(void* src) {
            if (_size == _capacity) {
                reserve(_capacity == 0 ? 1 : _capacity * 2);
            }

            _info._constructor(_buffer + _size * _info._size, src);
            ++_size;
        }

        /**
         * @brief Removes the last element from the container by calling the destructor function from the type information
         * on the last element. The destructor function is expected to properly destruct the element in place, given a
         * pointer to the element to be destructed.
         */
        void pop_back() noexcept {
            --_size;
            _info._destructor(_buffer + _size * _info._size);
        }

        /**
         * @brief Swaps the elements at the specified indices by calling the swapper function from the type information on
         * the elements at the given indices. The swapper function is expected to swap the elements in place, given pointers
         * to the two elements to be swapped.
         * 
         * @param lh The index of the first element to swap.
         * @param rh The index of the second element to swap.
         */
        void swap(size_type lh, size_type rh) noexcept {
            _info._swapper(_buffer + lh * _info._size, _buffer + rh * _info._size);
        }

        /**
         * @brief Clears the container by calling the destructor function from the type information on all elements in the
         * container, properly destructing them in place, and then setting the size of the container to zero. The destructor
         * function is expected to properly destruct each element in place, given a pointer to the element to be destructed.
         */
        void clear() noexcept {
            for (size_type i = 0; i < _size; ++i) {
                _info._destructor(_buffer + i * _info._size);
            }

            _size = 0;
        }

        /**
         * @brief Reduces the capacity of the container to match its size, deallocating any excess memory.
         */
        void shrink_to_fit() {
            if (_capacity > _size) {
                buffer_type new_buffer = static_cast<buffer_type>(
                    ::operator new(_size * _info._size, std::align_val_t(_info._align))
                );

                for (size_type i = 0; i < _size; ++i) {
                    buffer_type src = _buffer + i * _info._size;
                    _info._constructor(new_buffer + i * _info._size, src);
                    _info._destructor(src);
                }

                ::operator delete(_buffer, std::align_val_t(_info._align));
                _buffer = new_buffer;
                _capacity = _size;
            }
        }

        /**
         * @brief Reserves capacity for at least the specified number of elements in the container. If the requested capacity
         * is greater than the current capacity, the container's capacity is increased to match the requested capacity.
         * 
         * @param n The minimum number of elements for which to reserve space.
         */
        void reserve(size_type n) {
            if (n > _capacity) {
                buffer_type new_buffer = static_cast<buffer_type>(
                    ::operator new(n * _info._size, std::align_val_t(_info._align))
                );

                for (size_type i = 0; i < _size; ++i) {
                    buffer_type src = _buffer + i * _info._size;
                    _info._constructor(new_buffer + i * _info._size, src);
                    _info._destructor(src);
                }

                ::operator delete(_buffer, std::align_val_t(_info._align));
                _buffer = new_buffer;
                _capacity = n;
            }
        }

    public:
        /** @brief Checks if the container is empty. */
        [[nodiscard]] bool empty() const noexcept { return _size == 0; }

        /** @brief Returns the number of elements in the container. */
        [[nodiscard]] size_type size() const noexcept { return _size; }

        /** @brief Returns the total capacity of the container. */
        [[nodiscard]] size_type capacity() const noexcept { return _capacity; }

        /** @brief Accesses the element at the specified index. */
        [[nodiscard]] void* operator[](size_type index) const noexcept {
            return static_cast<void*>(_buffer + index * _info._size);
        }

    private:
        buffer_type _buffer;
        size_type _size;
        size_type _capacity;
        const info_type& _info;
    };
} // namespace myth::core::container