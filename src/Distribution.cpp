#include "Distribution.h"
#include "Geometry.h"
#include "Random.h"
#include <cmath>
#include <random>



void NormalDistribution::calculate_covariance() {
    mean = Point{0, 0};
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

NormalDistribution::NormalDistribution(const covariance& cov, Point mean) : cov(cov), mean(mean) {}

NormalDistribution::NormalDistribution(std::vector<Point> points) : Distribution(std::move(points)) {
    calculate_covariance();
}

Point NormalDistribution::sample() const {
    std::normal_distribution<double> nd(0, 1);
    double z1 = nd(random_engine);
    double z2 = nd(random_engine);

    covariance cholesky{};
    cholesky[0][0] = std::sqrt(cov[0][0]);
    cholesky[1][0] = cov[0][1] / cholesky[0][0];
    cholesky[0][1] = 0;
    cholesky[1][1] = std::sqrt(cov[1][1] - cholesky[1][0] * cholesky[1][0]);

    return Point{
        mean.x + cholesky[0][0] * z1,
        mean.y + cholesky[1][0] * z1 + cholesky[1][1] * z2
    };
}

void NormalDistribution::add_point(Point p) {
    points.push_back(p);
    calculate_covariance();
}



double NormalDistributionRandom::integrate_probability(const Polygon& region) const {
    return integrate_probability(region, PointDifference{0, 0});
}

double NormalDistributionRandom::integrate_probability(const Polygon& region, PointDifference offset) const {
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

double NormalDistributionQuadrature::integrate_probability(const Polygon& region, PointDifference offset) const {
    
}
