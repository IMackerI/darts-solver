#include <gtest/gtest.h>
#include "Geometry.h"

typedef Vec2 P;

TEST(Polygon, SimpleConvex) {
    // Unit square
    Polygon square(std::vector<P>{P{0, 0}, P{1, 0}, P{1, 1}, P{0, 1}});
    EXPECT_TRUE(square.contains(P{0.5, 0.5}));
    EXPECT_FALSE(square.contains(P{1.5, 0.5}));
    EXPECT_FALSE(square.contains(P{-0.5, 0.5}));
}

TEST(Polygon, NonConvex) {
    // L-shaped polygon (non-convex)
    Polygon L(std::vector<P>{P{0, 0}, P{2, 0}, P{2, 1}, P{1, 1}, P{1, 2}, P{0, 2}});
    
    // Inside points
    EXPECT_TRUE(L.contains(P{0.5, 0.5}));
    EXPECT_TRUE(L.contains(P{0.5, 1.5}));
    EXPECT_TRUE(L.contains(P{1.5, 0.5}));
    
    // Outside in the concave region
    EXPECT_FALSE(L.contains(P{1.5, 1.5}));
}

TEST(Polygon, EdgeCases) {
    Polygon triangle(std::vector<P>{P{0, 0}, P{2, 0}, P{1, 2}});
    
    // Clearly inside (not on boundary)
    EXPECT_TRUE(triangle.contains(P{1, 0.5}));
    EXPECT_TRUE(triangle.contains(P{1, 1}));
    EXPECT_TRUE(triangle.contains(P{0.5, 0.25}));
    
    // Clearly outside
    EXPECT_FALSE(triangle.contains(P{-1, 0}));
    EXPECT_FALSE(triangle.contains(P{1, 3}));
    EXPECT_FALSE(triangle.contains(P{3, 0}));
    EXPECT_FALSE(triangle.contains(P{-0.5, -0.5}));
    EXPECT_FALSE(triangle.contains(P{2.5, 0.5}));
}

TEST(Polygon, ComplexNonConvex) {
    // Star shape (highly non-convex)
    Polygon star(std::vector<P>{
        P{0, -2}, P{0.5, -0.5}, P{2, 0}, P{0.5, 0.5},
        P{0, 2}, P{-0.5, 0.5}, P{-2, 0}, P{-0.5, -0.5}
    });
    
    // Center
    EXPECT_TRUE(star.contains(P{0, 0}));
    
    // In the spikes
    EXPECT_TRUE(star.contains(P{1.5, 0}));
    EXPECT_TRUE(star.contains(P{0, 1.5}));
    
    // In the concave regions (between spikes)
    EXPECT_FALSE(star.contains(P{1, 1}));
    EXPECT_FALSE(star.contains(P{-1, -1}));
}
