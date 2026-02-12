#ifndef GEOMETRY_HEADER
#define GEOMETRY_HEADER

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