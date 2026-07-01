#pragma once

#include <cstddef>
#include <concepts>
#include <type_traits>
#include <tuple>
#include <utility>

#include "core/concept.hpp"

namespace myth::core {
    namespace internal {
        /**
         * @brief A helper class template that represents an element of a compressed pair. It has two specializations: one for types that are
         * not eligible for Empty Base Class Optimization (EBCO), which stores the value directly, and one for EBCO-eligible types, which inherits
         * from the type to take advantage of EBCO.
         * 
         * @tparam Type The type of the element.
         * @tparam Tag A unique tag to distinguish between different elements in the compressed pair, used to avoid ambiguity in the case of multiple
         * EBCO-eligible types.
         */
        template<typename Type, size_t>
        struct compressed_pair_element {
        public:
            /** @brief The type of the value stored in the element. */
            using value_type = Type;

            /** @brief Constructs a default instance of the element. */
            constexpr compressed_pair_element() noexcept(std::is_nothrow_default_constructible_v<value_type>)
            requires (std::default_initializable<value_type>) {}

            /** @brief Constructs an instance of the element with the given argument. */
            template<typename Arg>
            constexpr compressed_pair_element(Arg&& arg) noexcept(std::is_nothrow_constructible_v<value_type, Arg>)
            requires (!std::same_as<std::remove_cvref_t<Arg>, compressed_pair_element>)
                : _value{std::forward<Arg>(arg)} {}

            /** @brief Constructs an instance of the element with the given arguments. */
            template<typename... Args, size_t... Index>
            constexpr compressed_pair_element(std::tuple<Args...> args, std::index_sequence<Index...>) noexcept(std::is_nothrow_constructible_v<value_type, Args...>)
                : _value{std::forward<Args>(std::get<Index>(args))...} {}

        public:
            /** @brief Returns a reference to the value stored in the element. */
            [[nodiscard]] constexpr value_type& get() noexcept { return _value; }

            /** @brief Returns a const reference to the value stored in the element. */
            [[nodiscard]] constexpr const value_type& get() const noexcept { return _value; }

        private:
            value_type _value{};
        };

        /**
         * @brief A specialization of compressed_pair_element for EBCO-eligible types, which inherits from the type to take advantage of EBCO.
         * 
         * @tparam Type The type of the element, which must be EBCO-eligible.
         * @tparam Tag A unique tag to distinguish between different elements in the compressed pair,
         */
        template<EBCOEligibleType Type, size_t Tag>
        struct compressed_pair_element<Type, Tag> : Type {
            /** @brief The type of the base class. */
            using base_type = Type;

            /** @brief Constructs a default instance of the element. */
            constexpr compressed_pair_element() noexcept(std::is_nothrow_default_constructible_v<base_type>)
            requires (std::default_initializable<base_type>) {}

            /** @brief Constructs an instance of the element with the given argument. */
            template<typename Arg>
            constexpr compressed_pair_element(Arg&& arg) noexcept(std::is_nothrow_constructible_v<base_type, Arg>)
            requires (!std::same_as<std::remove_cvref_t<Arg>, compressed_pair_element>)
                : base_type{std::forward<Arg>(arg)} {}

            /** @brief Constructs an instance of the element with the given arguments. */
            template<typename... Args, size_t... Index>
            constexpr compressed_pair_element(std::tuple<Args...> args, std::index_sequence<Index...>) noexcept(std::is_nothrow_constructible_v<base_type, Args...>)
                : base_type{std::forward<Args>(std::get<Index>(args))...} {}

            /** @brief Returns a reference to the value stored in the element. */
            [[nodiscard]] constexpr base_type& get() noexcept { return *this; }

            /** @brief Returns a const reference to the value stored in the element. */
            [[nodiscard]] constexpr const base_type& get() const noexcept { return *this; }
        };
    } // namespace internal

    /**
     * @brief A class template that represents a pair of values, where the first and second values can be of different types. It uses Empty Base
     * Class Optimization (EBCO) to optimize the storage of the values when possible, by inheriting from the types if they are EBCO-eligible.
     * 
     * @tparam First The type of the first value in the pair.
     * @tparam Second The type of the second value in the pair.
     */
    template<typename First, typename Second>
    class compressed_pair final : internal::compressed_pair_element<First, 0u>, internal::compressed_pair_element<Second, 1u> {
        /** @brief The type of the base class for the first element. */
        using first_base_type = internal::compressed_pair_element<First, 0u>;
        /** @brief The type of the base class for the second element. */
        using second_base_type = internal::compressed_pair_element<Second, 1u>;

    public:
        /** @brief The type of the first value in the pair. */
        using first_type = First;
        /** @brief The type of the second value in the pair. */
        using second_type = Second;

        /** @brief Constructs a default instance of the compressed pair. */
        constexpr compressed_pair() noexcept(std::is_nothrow_default_constructible_v<First> && std::is_nothrow_default_constructible_v<Second>)
        requires (std::default_initializable<First> && std::default_initializable<Second>) {}

        /** @brief Constructs an instance of the compressed pair with the given arguments. */
        template<typename Arg, typename Other>
        constexpr compressed_pair(Arg&& arg, Other&& other) noexcept(std::is_nothrow_constructible_v<First, Arg> && std::is_nothrow_constructible_v<Second, Other>)
            : first_base_type{std::forward<Arg>(arg)}, second_base_type{std::forward<Other>(other)} {}

        /** @brief Constructs an instance of the compressed pair with the given arguments, using piecewise construction. */
        template<typename... Args, typename... Others>
        constexpr compressed_pair(std::piecewise_construct_t, std::tuple<Args...> args, std::tuple<Others...> others)
            noexcept(std::is_nothrow_constructible_v<first_base_type, Args...> && std::is_nothrow_constructible_v<second_base_type, Others...>)
            : first_base_type{std::move(args), std::index_sequence_for<Args...>{}}, second_base_type{std::move(others), std::index_sequence_for<Others...>{}} {}

        /** @brief Constructs a copy of the compressed pair. */
        constexpr compressed_pair(const compressed_pair&) = default;

        /** @brief Constructs a move of the compressed pair. */
        constexpr compressed_pair(compressed_pair&&) noexcept = default;

        /** @brief Destructs the compressed pair. */
        constexpr ~compressed_pair() noexcept = default;

        /** @brief Assigns a copy of the compressed pair. */
        constexpr compressed_pair& operator=(const compressed_pair&) = default;

        /** @brief Assigns a move of the compressed pair. */
        constexpr compressed_pair& operator=(compressed_pair&&) noexcept = default;

    public:
        /**
         * @brief Swaps the contents of this compressed pair with another one.
         * 
         * @param other The other compressed pair to swap with.
         */
        constexpr void swap(compressed_pair& other) noexcept {
            std::swap(first(), other.first());
            std::swap(second(), other.second());
        }

    public:
        /** @brief Returns a reference to the first value in the pair. */
        [[nodiscard]] constexpr first_type& first() noexcept { return first_base_type::get(); }

        /** @brief Returns a const reference to the first value in the pair. */
        [[nodiscard]] constexpr const first_type& first() const noexcept { return first_base_type::get(); }

        /** @brief Returns a reference to the second value in the pair. */
        [[nodiscard]] constexpr second_type& second() noexcept { return second_base_type::get(); }

        /** @brief Returns a const reference to the second value in the pair. */
        [[nodiscard]] constexpr const second_type& second() const noexcept { return second_base_type::get(); }

        /** @brief Returns a reference to the value at the specified index (0 for first, 1 for second). */
        template<size_t Index>
        requires (Index <= 1u)
        [[nodiscard]] constexpr decltype(auto) get() noexcept {
            if constexpr (Index == 0) {
                return first();
            } else {
                return second();
            }
        }

        /** @brief Returns a const reference to the value at the specified index (0 for first, 1 for second). */
        template<size_t Index>
        requires (Index <= 1u)
        [[nodiscard]] constexpr decltype(auto) get() const noexcept {
            if constexpr (Index == 0) {
                return first();
            } else {
                return second();
            }
        }
    };

    /** @brief Deduction guide for constructing a compressed pair with two arguments. */
    template<typename Arg, typename Other>
    compressed_pair(Arg&&, Other&&) -> compressed_pair<std::decay_t<Arg>, std::decay_t<Other>>;
} // namespace myth::core

namespace std {
    /** @brief Specialization of `tuple_size` for `myth::core::compressed_pair`. */
    template<typename First, typename Second>
    struct tuple_size<myth::core::compressed_pair<First, Second>> : std::integral_constant<size_t, 2u> {};

    /** @brief Specialization of `tuple_element` for `myth::core::compressed_pair`. */
    template<size_t Index, typename First, typename Second>
    requires (Index <= 1u)
    struct tuple_element<Index, myth::core::compressed_pair<First, Second>> : std::conditional<Index == 0u, First, Second> {};
} // namespace std