#include <gtest/gtest.h>
#include <core/concept.hpp>

TEST(Concept, UnsignedIntegralType) {
    EXPECT_TRUE((myth::core::UnsignedIntegralType<unsigned int>));
    EXPECT_TRUE((myth::core::UnsignedIntegralType<unsigned long>));
    EXPECT_TRUE((myth::core::UnsignedIntegralType<unsigned long long>));
    EXPECT_TRUE((myth::core::UnsignedIntegralType<std::uint8_t>));
    EXPECT_TRUE((myth::core::UnsignedIntegralType<std::uint16_t>));
    EXPECT_TRUE((myth::core::UnsignedIntegralType<std::uint32_t>));
    EXPECT_TRUE((myth::core::UnsignedIntegralType<std::uint64_t>));

    EXPECT_FALSE((myth::core::UnsignedIntegralType<int>));
    EXPECT_FALSE((myth::core::UnsignedIntegralType<long>));
    EXPECT_FALSE((myth::core::UnsignedIntegralType<long long>));
    EXPECT_FALSE((myth::core::UnsignedIntegralType<std::int8_t>));
    EXPECT_FALSE((myth::core::UnsignedIntegralType<std::int16_t>));
    EXPECT_FALSE((myth::core::UnsignedIntegralType<std::int32_t>));
    EXPECT_FALSE((myth::core::UnsignedIntegralType<std::int64_t>));

    EXPECT_FALSE((myth::core::UnsignedIntegralType<float>));
    EXPECT_FALSE((myth::core::UnsignedIntegralType<double>));
    EXPECT_FALSE((myth::core::UnsignedIntegralType<char*>));
    EXPECT_FALSE((myth::core::UnsignedIntegralType<bool>));

    struct MyStruct {};
    EXPECT_FALSE((myth::core::UnsignedIntegralType<MyStruct>));
}