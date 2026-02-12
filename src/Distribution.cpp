#include "Distribution.h"
#include "Geometry.h"
#include "Random.h"
#include <cmath>
#include <random>


void NormalDistribution::calculate_covariance() {
    mean = Vec2{0, 0};
    cov = {{{0, 0}, {0, 0}}};

    for (const auto& p : points) {
        mean.x += p.x;
        mean.y += p.y;
    }
    mean.x /= points.size();
    mean.y /= points.size();

    for (const auto& p : points) {
        cov[0][0] += (p.x - mean.x) * (p.x - mean.x);
        cov[0][1] += (p.x - mean.x) * (p.y - mean.y);
        cov[1][0] += (p.y - mean.y) * (p.x - mean.x);
        cov[1][1] += (p.y - mean.y) * (p.y - mean.y);
    }
    cov[0][0] /= points.size();
    cov[0][1] /= points.size();
    cov[1][0] /= points.size();
    cov[1][1] /= points.size();
}

double NormalDistribution::cov_determinant() const {
    return cov[0][0] * cov[1][1] - cov[0][1] * cov[1][0];
}

NormalDistribution::covariance NormalDistribution::cov_inverse() const {
    double det = cov_determinant();
    return {{{cov[1][1] / det, -cov[0][1] / det}, {-cov[1][0] / det, cov[0][0] / det}}};
}

NormalDistribution::NormalDistribution(const covariance& cov, Vec2 mean) : cov(cov), mean(mean) {}

NormalDistribution::NormalDistribution(std::vector<Vec2> points) : Distribution(std::move(points)) {
    calculate_covariance();
}

double NormalDistribution::probability_density(Vec2 p) const {
    double c = 1.0 / (2 * M_PI * std::sqrt(cov_determinant()));
    Vec2 diff = p - mean;
    covariance inv_cov = cov_inverse();
    double exponent = -0.5 * (
        diff.x * (inv_cov[0][0] * diff.x + inv_cov[0][1] * diff.y) +
        diff.y * (inv_cov[1][0] * diff.x + inv_cov[1][1] * diff.y)
    );
    return c * std::exp(exponent);
}

Vec2 NormalDistribution::sample() const {
    std::normal_distribution<double> nd(0, 1);
    double z1 = nd(random_engine);
    double z2 = nd(random_engine);

    covariance cholesky{};
    cholesky[0][0] = std::sqrt(cov[0][0]);
    cholesky[1][0] = cov[0][1] / cholesky[0][0];
    cholesky[0][1] = 0;
    cholesky[1][1] = std::sqrt(cov[1][1] - cholesky[1][0] * cholesky[1][0]);

    return Vec2{
        mean.x + cholesky[0][0] * z1,
        mean.y + cholesky[1][0] * z1 + cholesky[1][1] * z2
    };
}

void NormalDistribution::add_point(Vec2 p) {
    points.push_back(p);
    calculate_covariance();
}



double NormalDistributionRandom::integrate_probability(const Polygon& region) const {
    return integrate_probability(region, Vec2{0, 0});
}

double NormalDistributionRandom::integrate_probability(const Polygon& region, Vec2 offset) const {
    size_t count = 0;
    for (size_t i = 0; i < num_samples; ++i) {
        if (region.contains(sample() + offset)) {
            count++;
        }
    }
    return static_cast<double>(count) / num_samples;
}


double NormalDistributionQuadrature::integrate_probability(const Polygon& region) const {
    
}

double NormalDistributionQuadrature::integrate_probability(const Polygon& region, Vec2 offset) const {
    
}
