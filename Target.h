#ifndef TARGET_HEADER
#define TARGET_HEADER

#include "Geometry.h"
#include <vector>
#include <fstream>

using state = unsigned int;
using score = double;

class Bed {
    Polygon shape;
public:
    Bed(const Polygon& shape);
    void import(std::istream &input);

    bool inside(const Point& p) const;
    state after_hit(const state& original_state) const;
};

class Target {
    std::vector<Bed> beds;
public:
    Target(const std::vector<Bed>& beds);
    void import(std::istream &input);
    void import(const std::string &filename);
};

#endif