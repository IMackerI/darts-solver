#ifndef GEOMETRY_HEADER
#define GEOMETRY_HEADER

#include <vector>

struct Point {
    double x;
    double y;
};

class Polygon {
private:
    bool ray_point_intersect(Point p, Point a, Point b) const;
public:
    std::vector<Point> vertices;

    bool contains(Point p) const;
};

#endif