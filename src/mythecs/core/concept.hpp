#pragma once

#include <type_traits>

namespace myth::core {
    /**
     * @brief A concept that represents an unsigned integral type, excluding bool.
     * @tparam T The type to check.
     */
    template<typename T>
    concept UnsignedIntegralType = std::is_integral_v<T> && std::is_unsigned_v<T> && !std::is_same_v<T, bool>;
} // namespace myth::core