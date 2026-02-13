#ifndef GEOMETRY_HEADER
#define GEOMETRY_HEADER

#include <cstddef>
#include <cstdlib>
#include <vector>

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
    template <>
    struct hash<Vec2> {
        size_t operator()(const Vec2& v) const {
            size_t h1 = hash<double>{}(v.x);
            size_t h2 = hash<double>{}(v.y);
            return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        }
    };
}

inline double triangle_area(Vec2 v0, Vec2 v1, Vec2 v2) {
    return 0.5 * std::abs((v1.x - v0.x) * (v2.y - v0.y) - (v2.x - v0.x) * (v1.y - v0.y));
}

class Polygon {
private:
    std::vector<Vec2> vertices_;
    static bool ray_segment_intersect_(Vec2 p, Vec2 a, Vec2 b);
public:
    Polygon() = default;
    Polygon(std::vector<Vec2>&& vertices);
    Polygon(const std::vector<Vec2>& vertices);
    
    [[nodiscard]] bool contains(Vec2 p) const;
    [[nodiscard]] const std::vector<Vec2>& get_vertices() const { return vertices_; }
    void set_vertices(std::vector<Vec2>&& v) { vertices_ = std::move(v); }
};

#endif