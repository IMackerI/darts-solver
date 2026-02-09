#include "Distribution.h"
#include "Random.h"
#include <random>



void NormalDistributionRandom::calculate_covariance() {
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

NormalDistributionRandom::NormalDistributionRandom(const covariance& cov, const Point& mean) : cov(cov), mean(mean) {}

NormalDistributionRandom::NormalDistributionRandom(std::vector<Point> points) : points(std::move(points)) {
    calculate_covariance();
}

Point NormalDistributionRandom::sample() const {
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

double NormalDistributionRandom::integrate_probability(const Polygon& region) const {
    
}

void NormalDistributionRandom::add_point(const Point& p) {
    points.push_back(p);
    calculate_covariance();
}

DiscreteDistribution::DiscreteDistribution(size_t height_resolution, size_t width_resolution)
    : height_resolution(),
      width_resolution()
{
    
}

DiscreteDistribution::DiscreteDistribution(const std::vector<Point>& points, size_t height_resolution, size_t width_resolution)
    : height_resolution(),
      width_resolution()
{
    
}

Point DiscreteDistribution::sample() const {
    
}

double DiscreteDistribution::integrate_probability(const Polygon& region) const {
    
}

void DiscreteDistribution::add_point(const Point& p) {
    
}
