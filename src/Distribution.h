#ifndef DISTRIBUTION_HEADER
#define DISTRIBUTION_HEADER

#include "Geometry.h"

#include <cstddef>
#include <vector>
#include <array>

class Distribution {
protected:
    std::vector<Vec2> points_;

public:
    Distribution() = default;
    Distribution(std::vector<Vec2> points) : points_(std::move(points)) {}
    virtual ~Distribution() = default;
    virtual double probability_density(Vec2 p) const = 0;
    virtual Vec2 sample() const = 0;
    virtual double integrate_probability(const Polygon& region) const = 0;
    virtual double integrate_probability(const Polygon& region, Vec2 offset) const = 0;
    virtual void add_point(Vec2 p) = 0;
};

class NormalDistribution : public Distribution {
public:
    using covariance = std::array<std::array<double, 2>, 2>;
protected:
    covariance cov_;
    Vec2 mean_;

    void calculate_covariance_();
    double cov_determinant_() const;
    covariance cov_inverse_() const;
public:
    virtual ~NormalDistribution() = default;
    NormalDistribution(const covariance& cov, Vec2 mean = Vec2{0, 0});
    NormalDistribution(std::vector<Vec2> points);
    double probability_density(Vec2 p) const override;
    Vec2 sample() const override;
    virtual double integrate_probability(const Polygon& region) const override = 0;
    virtual double integrate_probability(const Polygon& region, Vec2 offset) const override = 0;
    void add_point(Vec2 p) override;
};

class NormalDistributionRandom final : public NormalDistribution {
private:
    size_t num_samples_;
public:
    NormalDistributionRandom(const covariance& cov, Vec2 mean = Vec2{0, 0}, size_t num_samples = 10000);
    NormalDistributionRandom(std::vector<Vec2> points, size_t num_samples = 1000);
    double integrate_probability(const Polygon& region) const override;
    double integrate_probability(const Polygon& region, Vec2 offset) const override;
    void set_integration_precision(size_t num_samples) {
        this->num_samples_ = num_samples;
    }
};

class NormalDistributionQuadrature final : public NormalDistribution {
    using NormalDistribution::NormalDistribution;
public:
    double integrate_probability(const Polygon& region) const override;
    double integrate_probability(const Polygon& region, Vec2 offset) const override;
};

#endif