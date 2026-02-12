#ifndef DISTRIBUTION_HEADER
#define DISTRIBUTION_HEADER

#include "Geometry.h"

#include <vector>
#include <array>

class Distribution {
protected:
    std::vector<Point> points;
public:
    Distribution() = default;
    Distribution(std::vector<Point> points) : points(std::move(points)) {}
    virtual ~Distribution() = default;
    virtual double probability_density(Point p) const = 0;
    virtual Point sample() const = 0;
    virtual double integrate_probability(const Polygon& region) const = 0;
    virtual double integrate_probability(const Polygon& region, PointDifference offset) const = 0;
    virtual void add_point(Point p) = 0;
};

class NormalDistribution : public Distribution {
public:
    using covariance = std::array<std::array<double, 2>, 2>;
protected:
    covariance cov;
    Point mean;

    void calculate_covariance();
    double cov_determinant() const;
    covariance cov_inverse() const;
public:
    virtual ~NormalDistribution() = default;
    NormalDistribution(const covariance& cov, Point mean = Point{0, 0});
    NormalDistribution(std::vector<Point> points);
    double probability_density(Point p) const override;
    Point sample() const override;
    virtual double integrate_probability(const Polygon& region) const override = 0;
    virtual double integrate_probability(const Polygon& region, PointDifference offset) const override = 0;
    void add_point(Point p) override;
};

class NormalDistributionRandom final : public NormalDistribution {
private:
    size_t num_samples;
public:
    NormalDistributionRandom(const covariance& cov, Point mean = Point{0, 0}, size_t num_samples = 10000);
    NormalDistributionRandom(std::vector<Point> points, size_t num_samples = 1000);
    double integrate_probability(const Polygon& region) const override;
    double integrate_probability(const Polygon& region, PointDifference offset) const override;
    void set_integration_precision(size_t num_samples) {
        this->num_samples = num_samples;
    }
};

class NormalDistributionQuadrature final : public NormalDistribution {
    using NormalDistribution::NormalDistribution;
public:
    double integrate_probability(const Polygon& region) const override;
    double integrate_probability(const Polygon& region, PointDifference offset) const override;
};

// class DiscreteDistribution : public Distribution {
//     std::vector<Point> points;
//     const size_t height_resolution, width_resolution;
//     std::vector<std::vector<double>> probability_grid;
// public:
//     DiscreteDistribution(size_t height_resolution, size_t width_resolution);
//     DiscreteDistribution(
//         const std::vector<Point>& points,
//         size_t height_resolution = 200,
//         size_t width_resolution = 200
//     );
//     Point sample() const override;
//     double integrate_probability(const Polygon& region) const override;
//     double integrate_probability(const Polygon& region, PointDifference offset) const override;
//     void add_point(Point p) override;
// };

#endif