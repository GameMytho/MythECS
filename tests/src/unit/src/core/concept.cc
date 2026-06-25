#include <gtest/gtest.h>
#include <core/concept.hpp>

// ============================================================================
// UnsignedIntegralType - accepts unsigned integral types, rejects everything else
// ============================================================================
TEST(Concept, UnsignedIntegralType) {
    // Accepted: unsigned integral types.
    EXPECT_TRUE((myth::core::UnsignedIntegralType<unsigned int>));
    EXPECT_TRUE((myth::core::UnsignedIntegralType<unsigned long>));
    EXPECT_TRUE((myth::core::UnsignedIntegralType<unsigned long long>));
    EXPECT_TRUE((myth::core::UnsignedIntegralType<std::uint8_t>));
    EXPECT_TRUE((myth::core::UnsignedIntegralType<std::uint16_t>));
    EXPECT_TRUE((myth::core::UnsignedIntegralType<std::uint32_t>));
    EXPECT_TRUE((myth::core::UnsignedIntegralType<std::uint64_t>));

    // Rejected: signed integral types.
    EXPECT_FALSE((myth::core::UnsignedIntegralType<int>));
    EXPECT_FALSE((myth::core::UnsignedIntegralType<long>));
    EXPECT_FALSE((myth::core::UnsignedIntegralType<long long>));
    EXPECT_FALSE((myth::core::UnsignedIntegralType<std::int8_t>));
    EXPECT_FALSE((myth::core::UnsignedIntegralType<std::int16_t>));
    EXPECT_FALSE((myth::core::UnsignedIntegralType<std::int32_t>));
    EXPECT_FALSE((myth::core::UnsignedIntegralType<std::int64_t>));

    // Rejected: non-integral types.
    EXPECT_FALSE((myth::core::UnsignedIntegralType<float>));
    EXPECT_FALSE((myth::core::UnsignedIntegralType<double>));
    EXPECT_FALSE((myth::core::UnsignedIntegralType<char*>));
    EXPECT_FALSE((myth::core::UnsignedIntegralType<bool>));

    // Rejected: user-defined types.
    struct MyStruct {};
    EXPECT_FALSE((myth::core::UnsignedIntegralType<MyStruct>));
}

// ============================================================================
// EBCOEligibleType - accepts empty non-final classes, rejects everything else
// ============================================================================
TEST(Concept, EBCOEligibleType) {
    struct Empty {};
    struct NonEmpty { int x; };
    struct FinalEmpty final {};
    struct FinalNonEmpty final { int x; };

    // Empty + non-final is EBCO-eligible.
    EXPECT_TRUE((myth::core::EBCOEligibleType<Empty>));
    // Non-empty is not.
    EXPECT_FALSE((myth::core::EBCOEligibleType<NonEmpty>));
    // Final class is never EBCO-eligible (inheritance would be ill-formed).
    EXPECT_FALSE((myth::core::EBCOEligibleType<FinalEmpty>));
    EXPECT_FALSE((myth::core::EBCOEligibleType<FinalNonEmpty>));

    // Built-in types are not empty.
    EXPECT_FALSE((myth::core::EBCOEligibleType<int>));
    EXPECT_FALSE((myth::core::EBCOEligibleType<float>));
    EXPECT_FALSE((myth::core::EBCOEligibleType<double>));
    EXPECT_FALSE((myth::core::EBCOEligibleType<char*>));
    EXPECT_FALSE((myth::core::EBCOEligibleType<bool>));
}
