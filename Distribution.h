#ifndef DISTRIBUTION_HEADER
#define DISTRIBUTION_HEADER

#include "Geometry.h"

#include <vector>

class Distribution {
public:
    virtual ~Distribution() = 0;
    virtual Point sample() const = 0;
    virtual double integrate_probability(const Polygon& region) const = 0;
};

#endif