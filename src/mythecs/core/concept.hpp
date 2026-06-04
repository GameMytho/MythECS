#pragma once

#include <type_traits>

namespace myth::core {
    /**
     * @brief A concept that represents an unsigned integral type, excluding bool.
     * 
     * @tparam T The type to check.
     */
    template<typename T>
    concept UnsignedIntegralType = std::is_integral_v<T> && std::is_unsigned_v<T> && !std::is_same_v<T, bool>;

    /**
     * @brief A concept that represents a type that is eligible for Empty Base Class Optimization (EBCO). A type is EBCO-eligible
     * if it is an empty class and not a final class.
     * 
     * @tparam T The type to check.
     */
    template<typename T>
    concept EBCOEligibleType = std::is_empty_v<T> && !std::is_final_v<T>;
} // namespace myth::core