#include <ecs.hpp>
#include <gtest/gtest.h>

TEST(MythECS, add) {
    EXPECT_EQ(mecs::add(2, 3), 5);
}