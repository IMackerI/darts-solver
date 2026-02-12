#ifndef GEOMETRY_HEADER
#define GEOMETRY_HEADER

#include <vector>

struct Point;

using PointDifference = Point;

struct Point {
    double x;
    double y;

    explicit Point(double x = 0, double y = 0) : x(x), y(y) {}

    Point operator+(PointDifference diff) const {
        return Point{x + diff.x, y + diff.y};
    }
    Point operator-(PointDifference diff) const {
        return Point{x - diff.x, y - diff.y};
    }
};


class Polygon {
private:
    std::vector<Point> vertices;
    bool ray_point_intersect(Point p, Point a, Point b) const;
public:
    Polygon() = default;
    Polygon(std::vector<Point>&& vertices);
    Polygon(const std::vector<Point>& vertices);
    bool contains(Point p) const;
    const std::vector<Point>& get_vertices() const { return vertices; }
    void set_vertices(std::vector<Point>&& v) { vertices = std::move(v); }
};

#endif