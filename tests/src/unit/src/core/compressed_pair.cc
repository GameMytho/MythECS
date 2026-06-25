#include <gtest/gtest.h>
#include <utility>
#include <core/compressed_pair.hpp>

using namespace myth::core;

// ============================================================================
// Size - EBCO eliminates empty type storage overhead
// ============================================================================
TEST(CompressedPair, Size) {
    struct empty {};

    struct local {
        int x;
        empty e;
    };

    // No EBCO: two ints.
    ASSERT_EQ(sizeof(compressed_pair<int, int>), 2 * sizeof(int));
    // EBCO applied: empty type takes zero space when first or second.
    ASSERT_EQ(sizeof(compressed_pair<empty, int>), sizeof(int));
    ASSERT_EQ(sizeof(compressed_pair<int, empty>), sizeof(int));
    ASSERT_EQ(sizeof(compressed_pair<int, empty>), sizeof(compressed_pair<empty, int>));
    // Smaller than a regular struct or std::pair that cannot use EBCO.
    ASSERT_LT(sizeof(compressed_pair<int, empty>), sizeof(local));
    ASSERT_LT(sizeof(compressed_pair<int, empty>), sizeof(std::pair<int, empty>));
}

// ============================================================================
// Constructors - default, value-init, piecewise
// ============================================================================
TEST(CompressedPair, Constructor) {
    // Default construction value-initializes both members.
    compressed_pair<int, double> pair1;

    ASSERT_EQ(pair1.first(), 0);
    ASSERT_EQ(pair1.second(), 0.0);

    // Direct construction from values.
    compressed_pair<int, std::string> pair2 { 42, "hello" };

    ASSERT_EQ(pair2.first(), 42);
    ASSERT_EQ(pair2.second(), "hello");

    // Piecewise construction via tuple forwarding.
    compressed_pair<std::string, std::string> pair3 { std::piecewise_construct, std::make_tuple("first"), std::make_tuple("second") };

    ASSERT_EQ(pair3.first(), "first");
    ASSERT_EQ(pair3.second(), "second");
}

// ============================================================================
// Copy - copy ctor and copy assignment
// ============================================================================
TEST(CompressedPair, Copy) {
    compressed_pair<int, std::string> pair1 { 42, "hello" };

    ASSERT_EQ(pair1.first(), 42);
    ASSERT_EQ(pair1.second(), "hello");

    // Copy ctor.
    compressed_pair<int, std::string> pair2 { pair1 };

    ASSERT_EQ(pair2.first(), 42);
    ASSERT_EQ(pair2.second(), "hello");

    // Copy assignment.
    compressed_pair<int, std::string> pair3 = pair1;

    ASSERT_EQ(pair3.first(), 42);
    ASSERT_EQ(pair3.second(), "hello");
}

// ============================================================================
// Move - move ctor and move assignment
// ============================================================================
TEST(CompressedPair, Move) {
    compressed_pair<int, std::string> pair1 { 42, "hello" };

    ASSERT_EQ(pair1.first(), 42);
    ASSERT_EQ(pair1.second(), "hello");

    // Move ctor - string content transferred.
    compressed_pair<int, std::string> pair2 { std::move(pair1) };

    ASSERT_EQ(pair2.first(), 42);
    ASSERT_EQ(pair2.second(), "hello");

    // Move assignment.
    compressed_pair<int, std::string> pair3 = std::move(pair2);

    ASSERT_EQ(pair3.first(), 42);
    ASSERT_EQ(pair3.second(), "hello");
}

// ============================================================================
// Swap - member-wise swap
// ============================================================================
TEST(CompressedPair, Swap) {
    compressed_pair<int, std::string> pair1 { 42, "hello" };
    compressed_pair<int, std::string> pair2 { 100, "world" };

    ASSERT_EQ(pair1.first(), 42);
    ASSERT_EQ(pair1.second(), "hello");
    ASSERT_EQ(pair2.first(), 100);
    ASSERT_EQ(pair2.second(), "world");

    pair1.swap(pair2);

    ASSERT_EQ(pair1.first(), 100);
    ASSERT_EQ(pair1.second(), "world");
    ASSERT_EQ(pair2.first(), 42);
    ASSERT_EQ(pair2.second(), "hello");
}

// ============================================================================
// Deduction guide - CTAD infers template args from ctor arguments
// ============================================================================
TEST(CompressedPair, DeductionGuide) {
    compressed_pair pair { 42, 3.1415926 };

    ASSERT_EQ(pair.first(), 42);
    ASSERT_EQ(pair.second(), 3.1415926);
}

// ============================================================================
// Get - tuple protocol: get<I>(), structured bindings
// ============================================================================
TEST(CompressedPair, Get) {
    compressed_pair<int, std::string> pair { 42, "hello" };

    // tuple-like element access.
    ASSERT_EQ(pair.get<0>(), 42);
    ASSERT_EQ(pair.get<1>(), "hello");

    // Structured binding via tuple_size / tuple_element specialization.
    auto& [first, second] = pair;

    ASSERT_EQ(first, 42);
    ASSERT_EQ(second, "hello");

    // Mutation through structured bindings.
    first = 100;
    second = "world";

    // Const structured binding reads the mutated values.
    const auto& [cfirst, csecond] = pair;

    ASSERT_EQ(cfirst, 100);
    ASSERT_EQ(csecond, "world");
}
