#ifndef GEOMETRY_HEADER
#define GEOMETRY_HEADER

#include <cstddef>
#include <cstdlib>
#include <vector>

/**
 * @defgroup geometry Geometry
 * @brief Geometric primitives and operations.
 */

/**
 * @brief Simple 2D vector structure with basic arithmetic operations.
 * @ingroup geometry
 */
struct Vec2 {
    double x;
    double y;

    explicit Vec2(double x = 0, double y = 0) : x(x), y(y) {}

    Vec2 operator+(Vec2 diff) const {
        return Vec2{x + diff.x, y + diff.y};
    }
    Vec2 operator-(Vec2 diff) const {
        return Vec2{x - diff.x, y - diff.y};
    }
    Vec2 operator*(double scalar) const {
        return Vec2{x * scalar, y * scalar};
    }
    bool operator==(Vec2 other) const {
        return x == other.x && y == other.y;
    }
};

namespace std {
    /**
     * @ingroup geometry
     * @brief Hash function specialization for Vec2.
     * Uses standard hash combining with XOR and bit shifts.
     */
    template <>
    struct hash<Vec2> {
        size_t operator()(const Vec2& v) const {
            size_t h1 = hash<double>{}(v.x);
            size_t h2 = hash<double>{}(v.y);
            return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        }
    };
}

/**
 * @ingroup geometry
 * @brief Compute the area of a triangle using the cross product formula.
 * @param v0 First vertex
 * @param v1 Second vertex
 * @param v2 Third vertex
 * @return Absolute area of the triangle
 */
inline double triangle_area(Vec2 v0, Vec2 v1, Vec2 v2) {
    return 0.5 * std::abs((v1.x - v0.x) * (v2.y - v0.y) - (v2.x - v0.x) * (v1.y - v0.y));
}

/**
 * @ingroup geometry
 * @brief A simple polygon represented by vertices.
 */
class Polygon {
private:
    std::vector<Vec2> vertices_;
    
    /** @brief Helper for ray casting: checks if horizontal ray from p intersects segment [a,b). */
    static bool ray_segment_intersect_(Vec2 p, Vec2 a, Vec2 b);
public:
    Polygon() = default;
    Polygon(std::vector<Vec2>&& vertices);
    Polygon(const std::vector<Vec2>& vertices);
    
    /**
     * @brief Point-in-polygon test using ray casting algorithm.
     * Casts a horizontal ray from p to the right and counts intersections.
     * @param p Point to test
     * @return true if p is inside the polygon (odd number of intersections)
     */
    [[nodiscard]] bool contains(Vec2 p) const;
    [[nodiscard]] const std::vector<Vec2>& get_vertices() const { return vertices_; }
    void set_vertices(std::vector<Vec2>&& v) { vertices_ = std::move(v); }
};

#endif