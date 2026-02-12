#ifndef DISTRIBUTION_HEADER
#define DISTRIBUTION_HEADER

#include "Geometry.h"

#include <vector>
#include <array>

class Distribution {
public:
    virtual ~Distribution() = default;
    virtual Point sample() const = 0;
    virtual double integrate_probability(const Polygon& region) const = 0;
    virtual double integrate_probability(const Polygon& region, PointDifference offset) const = 0;
    virtual void add_point(Point p) = 0;
};

class NormalDistributionRandom : public Distribution {
public:
    using covariance = std::array<std::array<double, 2>, 2>;
private:
    covariance cov;
    Point mean;
    std::vector<Point> points;
    size_t num_samples;

    void calculate_covariance();
public:
    NormalDistributionRandom(const covariance& cov, Point mean = Point{0, 0}, size_t num_samples = 10000);
    NormalDistributionRandom(std::vector<Point> points, size_t num_samples = 10000);
    Point sample() const override;
    double integrate_probability(const Polygon& region) const override;
    double integrate_probability(const Polygon& region, PointDifference offset) const override;
    void add_point(Point p) override;
    void set_integration_precision(size_t num_samples) {
        this->num_samples = num_samples;
    }
};

class DiscreteDistribution : public Distribution {
    std::vector<Point> points;
    const size_t height_resolution, width_resolution;
    std::vector<std::vector<double>> probability_grid;
public:
    DiscreteDistribution(size_t height_resolution, size_t width_resolution);
    DiscreteDistribution(
        const std::vector<Point>& points,
        size_t height_resolution = 200,
        size_t width_resolution = 200
    );
    Point sample() const override;
    double integrate_probability(const Polygon& region) const override;
    double integrate_probability(const Polygon& region, PointDifference offset) const override;
    void add_point(Point p) override;
};

#endif