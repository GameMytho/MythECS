#include <gtest/gtest.h>
#include <utility>
#include <core/compressed_pair.hpp>

using namespace myth::core;

TEST(CompressedPair, Size) {
    struct empty {};

    struct local {
        int x;
        empty e;
    };

    ASSERT_EQ(sizeof(compressed_pair<int, int>), 2 * sizeof(int));
    ASSERT_EQ(sizeof(compressed_pair<empty, int>), sizeof(int));
    ASSERT_EQ(sizeof(compressed_pair<int, empty>), sizeof(int));
    ASSERT_EQ(sizeof(compressed_pair<int, empty>), sizeof(compressed_pair<empty, int>));
    ASSERT_LT(sizeof(compressed_pair<int, empty>), sizeof(local));
    ASSERT_LT(sizeof(compressed_pair<int, empty>), sizeof(std::pair<int, empty>));
}

TEST(CompressedPair, Constructor) {
    compressed_pair<int, double> pair1;

    ASSERT_EQ(pair1.first(), 0);
    ASSERT_EQ(pair1.second(), 0.0);

    compressed_pair<int, std::string> pair2 { 42, "hello" };

    ASSERT_EQ(pair2.first(), 42);
    ASSERT_EQ(pair2.second(), "hello");

    compressed_pair<std::string, std::string> pair3 { std::piecewise_construct, std::make_tuple("first"), std::make_tuple("second") };

    ASSERT_EQ(pair3.first(), "first");
    ASSERT_EQ(pair3.second(), "second");
}

TEST(CompressedPair, Copy) {
    compressed_pair<int, std::string> pair1 { 42, "hello" };

    ASSERT_EQ(pair1.first(), 42);
    ASSERT_EQ(pair1.second(), "hello");

    compressed_pair<int, std::string> pair2 { pair1 };

    ASSERT_EQ(pair2.first(), 42);
    ASSERT_EQ(pair2.second(), "hello");

    compressed_pair<int, std::string> pair3 = pair1;

    ASSERT_EQ(pair3.first(), 42);
    ASSERT_EQ(pair3.second(), "hello");
}

TEST(CompressedPair, Move) {
    compressed_pair<int, std::string> pair1 { 42, "hello" };

    ASSERT_EQ(pair1.first(), 42);
    ASSERT_EQ(pair1.second(), "hello");

    compressed_pair<int, std::string> pair2 { std::move(pair1) };

    ASSERT_EQ(pair2.first(), 42);
    ASSERT_EQ(pair2.second(), "hello");

    compressed_pair<int, std::string> pair3 = std::move(pair2);

    ASSERT_EQ(pair3.first(), 42);
    ASSERT_EQ(pair3.second(), "hello");
}

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

TEST(CompressedPair, DeductionGuide) {
    compressed_pair pair { 42, 3.1415926 };

    ASSERT_EQ(pair.first(), 42);
    ASSERT_EQ(pair.second(), 3.1415926);
}

TEST(CompressedPair, Get) {
    compressed_pair<int, std::string> pair { 42, "hello" };

    ASSERT_EQ(pair.get<0>(), 42);
    ASSERT_EQ(pair.get<1>(), "hello");

    auto& [first, second] = pair;

    ASSERT_EQ(first, 42);
    ASSERT_EQ(second, "hello");

    first = 100;
    second = "world";

    const auto& [cfirst, csecond] = pair;

    ASSERT_EQ(cfirst, 100);
    ASSERT_EQ(csecond, "world");
}