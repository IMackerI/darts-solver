#ifndef GEOMETRY_HEADER
#define GEOMETRY_HEADER

#include <vector>

struct Point {
    double x;
    double y;
};

struct Polygon {
    std::vector<Point> vertices;
};

#endif