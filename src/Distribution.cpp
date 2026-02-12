#include "Distribution.h"
#include "Geometry.h"
#include "Random.h"
#include <cmath>
#include <random>

// Dunavant rule 5 (degree 5, 7 quadrature points) for unit reference triangle
// (0,0)-(1,0)-(0,1). Expanded from compressed barycentric suborders [1,3,3].
// Weights sum to 1.0 (normalized to reference triangle area).
// Reference: Dunavant, "High Degree Efficient Symmetrical Gaussian Quadrature
// Rules for the Triangle", IJNME Vol 21, 1985, pp. 1129-1148.
static constexpr int QUAD_NPTS = 7;
static constexpr double quad_r[QUAD_NPTS] = {
    0.333333333333333,
    0.059715871789770, 0.470142064105115, 0.470142064105115,
    0.797426985353087, 0.101286507323456, 0.101286507323456
};
static constexpr double quad_s[QUAD_NPTS] = {
    0.333333333333333,
    0.470142064105115, 0.470142064105115, 0.059715871789770,
    0.101286507323456, 0.101286507323456, 0.797426985353087
};
static constexpr double quad_w[QUAD_NPTS] = {
    0.225000000000000,
    0.132394152788506, 0.132394152788506, 0.132394152788506,
    0.125939180544827, 0.125939180544827, 0.125939180544827
};

Vec2 ref_to_physical(Vec2 v0, Vec2 v1, Vec2 v2, double r, double s) {
    return Vec2{
        v0.x * (1.0 - r - s) + v1.x * r + v2.x * s,
        v0.y * (1.0 - r - s) + v1.y * r + v2.y * s
    };
}

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

NormalDistributionRandom::NormalDistributionRandom(const covariance& cov, Vec2 mean, size_t num_samples)
    : NormalDistribution(cov, mean), num_samples(num_samples) {}

NormalDistributionRandom::NormalDistributionRandom(std::vector<Vec2> points, size_t num_samples)
    : NormalDistribution(std::move(points)), num_samples(num_samples) {}

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
    return integrate_probability(region, Vec2{0, 0});
}

double NormalDistributionQuadrature::integrate_probability(const Polygon& region, Vec2 offset) const {
    const auto& verts = region.get_vertices();
    if (verts.size() < 3) return 0.0;

    double total = 0.0;

    // Fan triangulation from vertex 0 (works for convex polygons)
    for (size_t i = 1; i + 1 < verts.size(); ++i) {
        Vec2 v0 = verts[0], v1 = verts[i], v2 = verts[i + 1];
        double area = triangle_area(v0, v1, v2);

        for (int q = 0; q < QUAD_NPTS; ++q) {
            Vec2 p = ref_to_physical(v0, v1, v2, quad_r[q], quad_s[q]);
            total += area * quad_w[q] * probability_density(p - offset);
        }
    }

    return total;
}
