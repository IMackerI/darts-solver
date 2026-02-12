#include "Geometry.h"

bool Polygon::ray_point_intersect(Vec2 ray_origin, Vec2 seg_start, Vec2 seg_end) const {
    if (seg_start.y > seg_end.y) {
        std::swap(seg_start, seg_end);
    }
    
    // [) avoid double-counting
    if (ray_origin.y < seg_start.y || ray_origin.y >= seg_end.y) return false;

    double t = (ray_origin.y - seg_start.y) / (seg_end.y - seg_start.y);
    double x_intersect = seg_start.x + t * (seg_end.x - seg_start.x);
    
    return x_intersect >= ray_origin.x;
}

Polygon::Polygon(std::vector<Vec2>&& vertices) : vertices(std::move(vertices)) {}
Polygon::Polygon(const std::vector<Vec2>& vertices) : vertices(vertices) {}

bool Polygon::contains(Vec2 p) const {
    size_t intersections = 0;
    for (size_t i = 0; i < vertices.size(); ++i) {
        const Vec2& a = vertices[i];
        const Vec2& b = vertices[(i + 1) % vertices.size()];
        if (ray_point_intersect(p, a, b)) {
            intersections++;
        }
    }
    return intersections % 2 == 1;
}