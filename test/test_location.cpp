#include <gtest/gtest.h>
#include "data/location.h"

TEST(LocationTest, DefaultConstructor) {
    Location loc;
    EXPECT_DOUBLE_EQ(loc.lat, 0.0);
    EXPECT_DOUBLE_EQ(loc.lon, 0.0);
}

TEST(LocationTest, ParameterizedConstructor) {
    Location loc(10.5, 20.5);
    EXPECT_DOUBLE_EQ(loc.lat, 10.5);
    EXPECT_DOUBLE_EQ(loc.lon, 20.5);
}