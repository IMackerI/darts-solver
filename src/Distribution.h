#ifndef DISTRIBUTION_HEADER
#define DISTRIBUTION_HEADER

#include "Geometry.h"

#include <vector>
#include <array>

class Distribution {
public:
    virtual ~Distribution() = 0;
    virtual Point sample() const = 0;
    virtual double integrate_probability(const Polygon& region) const = 0;
    virtual void add_point(const Point& p) = 0;
};

class NormalDistributionRandom : public Distribution {
    using covariance = std::array<std::array<double, 2>, 2>;
    covariance cov;
    Point mean;
    std::vector<Point> points;
    void calculate_covariance();
public:
    NormalDistributionRandom(const covariance& cov, const Point& mean = {0, 0});
    NormalDistributionRandom(std::vector<Point> points);
    Point sample() const override;
    double integrate_probability(const Polygon& region) const override;
    void add_point(const Point& p) override;
};

class DiscreteDistribution : public Distribution {
    std::vector<Point> points;
    const size_t height_resolution, width_resolution;
    std::vector<std::vector<double>> probability_grid;
public:
    DiscreteDistribution(size_t height_resolution, size_t width_resolution);
    DiscreteDistribution(const std::vector<Point>& points, size_t height_resolution = 200, size_t width_resolution = 200);
    Point sample() const override;
    double integrate_probability(const Polygon& region) const override;
    void add_point(const Point& p) override;
};

#endif