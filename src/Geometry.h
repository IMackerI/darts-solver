#ifndef GEOMETRY_HEADER
#define GEOMETRY_HEADER

#include <vector>

struct Point {
    double x;
    double y;
};

class Polygon {
private:
    std::vector<Point> vertices;
    bool ray_point_intersect(Point p, Point a, Point b) const;
public:
    Polygon(std::vector<Point>&& vertices);
    Polygon(const std::vector<Point>& vertices);
    bool contains(Point p) const;
};

#endif