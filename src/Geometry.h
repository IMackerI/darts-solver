#ifndef GEOMETRY_HEADER
#define GEOMETRY_HEADER

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
};

inline double det(const Vec2& a, const Vec2& b) {
    return a.x * b.y - a.y * b.x;
}

inline double triangle_area(Vec2 v0, Vec2 v1, Vec2 v2) {
    return 0.5 * std::abs((v1.x - v0.x) * (v2.y - v0.y) - (v2.x - v0.x) * (v1.y - v0.y));
}

class Polygon {
private:
    std::vector<Vec2> vertices;
    bool ray_point_intersect(Vec2 p, Vec2 a, Vec2 b) const;
public:
    Polygon() = default;
    Polygon(std::vector<Vec2>&& vertices);
    Polygon(const std::vector<Vec2>& vertices);
    bool contains(Vec2 p) const;
    const std::vector<Vec2>& get_vertices() const { return vertices; }
    void set_vertices(std::vector<Vec2>&& v) { vertices = std::move(v); }
};

#endif