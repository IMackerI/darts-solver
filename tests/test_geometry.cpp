#include <gtest/gtest.h>
#include "Geometry.h"

TEST(Polygon, SimpleConvex) {
    // Unit square
    Polygon square({{0, 0}, {1, 0}, {1, 1}, {0, 1}});
    EXPECT_TRUE(square.contains({0.5, 0.5}));
    EXPECT_FALSE(square.contains({1.5, 0.5}));
    EXPECT_FALSE(square.contains({-0.5, 0.5}));
}

TEST(Polygon, NonConvex) {
    // L-shaped polygon (non-convex)
    Polygon L({{0, 0}, {2, 0}, {2, 1}, {1, 1}, {1, 2}, {0, 2}});
    
    // Inside points
    EXPECT_TRUE(L.contains({0.5, 0.5}));
    EXPECT_TRUE(L.contains({0.5, 1.5}));
    EXPECT_TRUE(L.contains({1.5, 0.5}));
    
    // Outside in the concave region
    EXPECT_FALSE(L.contains({1.5, 1.5}));
}

TEST(Polygon, EdgeCases) {
    Polygon triangle({{0, 0}, {2, 0}, {1, 2}});
    
    // Clearly inside (not on boundary)
    EXPECT_TRUE(triangle.contains({1, 0.5}));
    EXPECT_TRUE(triangle.contains({1, 1}));
    EXPECT_TRUE(triangle.contains({0.5, 0.25}));
    
    // Clearly outside
    EXPECT_FALSE(triangle.contains({-1, 0}));
    EXPECT_FALSE(triangle.contains({1, 3}));
    EXPECT_FALSE(triangle.contains({3, 0}));
    EXPECT_FALSE(triangle.contains({-0.5, -0.5}));
    EXPECT_FALSE(triangle.contains({2.5, 0.5}));
}

TEST(Polygon, ComplexNonConvex) {
    // Star shape (highly non-convex)
    Polygon star({
        {0, -2}, {0.5, -0.5}, {2, 0}, {0.5, 0.5},
        {0, 2}, {-0.5, 0.5}, {-2, 0}, {-0.5, -0.5}
    });
    
    // Center
    EXPECT_TRUE(star.contains({0, 0}));
    
    // In the spikes
    EXPECT_TRUE(star.contains({1.5, 0}));
    EXPECT_TRUE(star.contains({0, 1.5}));
    
    // In the concave regions (between spikes)
    EXPECT_FALSE(star.contains({1, 1}));
    EXPECT_FALSE(star.contains({-1, -1}));
}
