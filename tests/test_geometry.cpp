#include <gtest/gtest.h>
#include "Geometry.h"
#include <cmath>

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

// Vec2 operations tests
TEST(Vec2, Addition) {
    Vec2 v1(3.0, 4.0);
    Vec2 v2(1.0, 2.0);
    Vec2 result = v1 + v2;
    EXPECT_EQ(result.x, 4.0);
    EXPECT_EQ(result.y, 6.0);
}

TEST(Vec2, Subtraction) {
    Vec2 v1(5.0, 7.0);
    Vec2 v2(2.0, 3.0);
    Vec2 result = v1 - v2;
    EXPECT_EQ(result.x, 3.0);
    EXPECT_EQ(result.y, 4.0);
}

TEST(Vec2, ScalarMultiplication) {
    Vec2 v(2.0, 3.0);
    Vec2 result = v * 2.5;
    EXPECT_EQ(result.x, 5.0);
    EXPECT_EQ(result.y, 7.5);
}

TEST(Vec2, Equality) {
    Vec2 v1(1.5, 2.5);
    Vec2 v2(1.5, 2.5);
    Vec2 v3(1.5, 2.6);
    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 == v3);
}

TEST(Vec2, DefaultConstructor) {
    Vec2 v;
    EXPECT_EQ(v.x, 0.0);
    EXPECT_EQ(v.y, 0.0);
}

TEST(Vec2, ChainedOperations) {
    Vec2 v1(1.0, 2.0);
    Vec2 v2(3.0, 4.0);
    Vec2 v3(0.5, 0.5);
    Vec2 result = (v1 + v2) * 2.0 - v3;
    EXPECT_EQ(result.x, 7.5);
    EXPECT_EQ(result.y, 11.5);
}

// triangle_area tests
TEST(TriangleArea, RightTriangle) {
    // Right triangle with legs 3 and 4, area should be 6
    Vec2 v0(0, 0);
    Vec2 v1(3, 0);
    Vec2 v2(0, 4);
    EXPECT_DOUBLE_EQ(triangle_area(v0, v1, v2), 6.0);
}

TEST(TriangleArea, EquilateralTriangle) {
    // Equilateral triangle with side length 2
    Vec2 v0(0, 0);
    Vec2 v1(2, 0);
    Vec2 v2(1, std::sqrt(3));
    EXPECT_NEAR(triangle_area(v0, v1, v2), std::sqrt(3), 1e-10);
}

TEST(TriangleArea, DegenerateTriangle) {
    // Collinear points (zero area)
    Vec2 v0(0, 0);
    Vec2 v1(1, 1);
    Vec2 v2(2, 2);
    EXPECT_NEAR(triangle_area(v0, v1, v2), 0.0, 1e-10);
}

TEST(TriangleArea, OrderIndependence) {
    // Area should be the same regardless of vertex order
    Vec2 v0(1, 1);
    Vec2 v1(4, 2);
    Vec2 v2(2, 5);
    double area1 = triangle_area(v0, v1, v2);
    double area2 = triangle_area(v1, v2, v0);
    double area3 = triangle_area(v2, v0, v1);
    EXPECT_DOUBLE_EQ(area1, area2);
    EXPECT_DOUBLE_EQ(area2, area3);
}

TEST(TriangleArea, NegativeCoordinates) {
    Vec2 v0(-2, -3);
    Vec2 v1(4, -1);
    Vec2 v2(1, 5);
    // Using cross product formula: 0.5 * |(4-(-2))(5-(-3)) - (1-(-2))((-1)-(-3))|
    // = 0.5 * |6*8 - 3*2| = 0.5 * |48 - 6| = 21
    EXPECT_DOUBLE_EQ(triangle_area(v0, v1, v2), 21.0);
}

// Additional Polygon tests
TEST(Polygon, EmptyPolygon) {
    Polygon empty(std::vector<P>{});
    // Should not contain any point
    EXPECT_FALSE(empty.contains(P{0, 0}));
    EXPECT_FALSE(empty.contains(P{1, 1}));
}

TEST(Polygon, GetVertices) {
    std::vector<P> vertices = {P{0, 0}, P{1, 0}, P{1, 1}, P{0, 1}};
    Polygon square(vertices);
    const auto& retrieved = square.get_vertices();
    EXPECT_EQ(retrieved.size(), 4);
    EXPECT_EQ(retrieved[0].x, 0);
    EXPECT_EQ(retrieved[0].y, 0);
}

TEST(Polygon, SetVertices) {
    Polygon poly;
    std::vector<P> vertices = {P{0, 0}, P{2, 0}, P{1, 2}};
    poly.set_vertices(std::move(vertices));
    EXPECT_TRUE(poly.contains(P{1, 0.5}));
}

TEST(Polygon, Pentagon) {
    // Regular pentagon approximation
    Polygon pentagon(std::vector<P>{
        P{0, 1}, P{0.95, 0.31}, P{0.59, -0.81}, 
        P{-0.59, -0.81}, P{-0.95, 0.31}
    });
    EXPECT_TRUE(pentagon.contains(P{0, 0}));
    EXPECT_FALSE(pentagon.contains(P{2, 2}));
}
