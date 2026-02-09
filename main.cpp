#include "src/Distribution.h"
#include "src/Geometry.h"
#include "src/Random.h"

#include <iostream>

int main() {
    NormalDistributionRandom::covariance cov = {{{1, 0.5}, {0.5, 1}}};
    NormalDistributionRandom dist(cov, {0, 0}, 100000);
    Polygon region({{0, 0}, {1, 0}, {1, 1}, {0, 1}});
    std::cout << "Estimated probability: " << dist.integrate_probability(region) << std::endl;
}