#include "Distribution.h"
#include "Geometry.h"
#include "Random.h"
#include <cmath>
#include <random>

namespace {
    // Dunavant rule 14 (42 quadrature points) for unit reference triangle
    // (0,0)-(1,0)-(0,1). Expanded from compressed barycentric suborders.
    // Weights sum to 1.0 (normalized to reference triangle area).
    // Reference: Dunavant, "High Degree Efficient Symmetrical Gaussian Quadrature
    // Rules for the Triangle", IJNME Vol 21, 1985, pp. 1129-1148.
    constexpr int QUAD_NPTS = 42;
    constexpr double quad_r[QUAD_NPTS] = {
        0.022072179275643, 0.488963910362179, 0.488963910362179,
        0.164710561319092, 0.417644719340454, 0.417644719340454,
        0.273477528308839, 0.273477528308839, 0.453044943382323,
        0.177205532412543, 0.177205532412543, 0.645588935174913,
        0.061799883090873, 0.061799883090873, 0.876400233818255,
        0.019390961248701, 0.019390961248701, 0.961218077502598,
        0.057124757403648, 0.057124757403648, 0.172266687821356,
        0.172266687821356, 0.770608554774996, 0.770608554774996,
        0.092916249356972, 0.092916249356972, 0.336861459796345,
        0.336861459796345, 0.570222290846683, 0.570222290846683,
        0.014646950055654, 0.014646950055654, 0.298372882136258,
        0.298372882136258, 0.686980167808088, 0.686980167808088,
        0.001268330932872, 0.001268330932872, 0.118974497696957,
        0.118974497696957, 0.879757171370171, 0.879757171370171,
    };
    constexpr double quad_s[QUAD_NPTS] = {
        0.488963910362179, 0.022072179275643, 0.488963910362179,
        0.417644719340454, 0.164710561319092, 0.417644719340454,
        0.273477528308839, 0.453044943382323, 0.273477528308839,
        0.177205532412543, 0.645588935174913, 0.177205532412543,
        0.061799883090873, 0.876400233818255, 0.061799883090873,
        0.019390961248701, 0.961218077502598, 0.019390961248701,
        0.172266687821356, 0.770608554774996, 0.057124757403648,
        0.770608554774996, 0.057124757403648, 0.172266687821356,
        0.336861459796345, 0.570222290846683, 0.092916249356972,
        0.570222290846683, 0.092916249356972, 0.336861459796345,
        0.298372882136258, 0.686980167808088, 0.014646950055654,
        0.686980167808088, 0.014646950055654, 0.298372882136258,
        0.118974497696957, 0.879757171370171, 0.001268330932872,
        0.879757171370171, 0.001268330932872, 0.118974497696957,
    };
    constexpr double quad_w[QUAD_NPTS] = {
        0.021883581369429, 0.021883581369429, 0.021883581369429,
        0.032788353544125, 0.032788353544125, 0.032788353544125,
        0.051774104507292, 0.051774104507292, 0.051774104507292,
        0.042162588736993, 0.042162588736993, 0.042162588736993,
        0.014433699669777, 0.014433699669777, 0.014433699669777,
        0.004923403602400, 0.004923403602400, 0.004923403602400,
        0.024665753212564, 0.024665753212564, 0.024665753212564,
        0.024665753212564, 0.024665753212564, 0.024665753212564,
        0.038571510787061, 0.038571510787061, 0.038571510787061,
        0.038571510787061, 0.038571510787061, 0.038571510787061,
        0.014436308113534, 0.014436308113534, 0.014436308113534,
        0.014436308113534, 0.014436308113534, 0.014436308113534,
        0.005010228838501, 0.005010228838501, 0.005010228838501,
        0.005010228838501, 0.005010228838501, 0.005010228838501,
    };

    Vec2 ref_to_physical(Vec2 v0, Vec2 v1, Vec2 v2, double r, double s) {
        return Vec2{
            v0.x * (1.0 - r - s) + v1.x * r + v2.x * s,
            v0.y * (1.0 - r - s) + v1.y * r + v2.y * s
        };
    }
}

void NormalDistribution::calculate_covariance_() {
    mean_ = Vec2{0.0, 0.0};
    cov_ = {{{0.0, 0.0}, {0.0, 0.0}}};

    for (const auto& p : points_) {
        mean_.x += p.x;
        mean_.y += p.y;
    }
    mean_.x /= points_.size();
    mean_.y /= points_.size();

    for (const auto& p : points_) {
        cov_[0][0] += (p.x - mean_.x) * (p.x - mean_.x);
        cov_[0][1] += (p.x - mean_.x) * (p.y - mean_.y);
        cov_[1][0] += (p.y - mean_.y) * (p.x - mean_.x);
        cov_[1][1] += (p.y - mean_.y) * (p.y - mean_.y);
    }
    cov_[0][0] /= points_.size();
    cov_[0][1] /= points_.size();
    cov_[1][0] /= points_.size();
    cov_[1][1] /= points_.size();
}

double NormalDistribution::cov_determinant_() const {
    return cov_[0][0] * cov_[1][1] - cov_[0][1] * cov_[1][0];
}

NormalDistribution::covariance NormalDistribution::cov_inverse_() const {
    double det = cov_determinant_();
    return {{{cov_[1][1] / det, -cov_[0][1] / det}, {-cov_[1][0] / det, cov_[0][0] / det}}};
}

NormalDistribution::NormalDistribution(const covariance& cov, Vec2 mean) : cov_(cov), mean_(mean) {}

NormalDistribution::NormalDistribution(std::vector<Vec2> points) : Distribution(std::move(points)) {
    calculate_covariance_();
}

double NormalDistribution::probability_density(Vec2 p) const {
    double c = 1.0 / (2 * M_PI * std::sqrt(cov_determinant_()));
    Vec2 diff = p - mean_;
    covariance inv_cov = cov_inverse_();
    double exponent = -0.5 * (
        diff.x * (inv_cov[0][0] * diff.x + inv_cov[0][1] * diff.y) +
        diff.y * (inv_cov[1][0] * diff.x + inv_cov[1][1] * diff.y)
    );
    return c * std::exp(exponent);
}

Vec2 NormalDistribution::sample() const {
    std::normal_distribution<double> nd(0.0, 1.0);
    double z1 = nd(random_engine);
    double z2 = nd(random_engine);

    // Cholesky decomposition of covariance matrix
    covariance cholesky{};
    cholesky[0][0] = std::sqrt(cov_[0][0]);
    cholesky[1][0] = cov_[0][1] / cholesky[0][0];
    cholesky[0][1] = 0.0;
    cholesky[1][1] = std::sqrt(cov_[1][1] - cholesky[1][0] * cholesky[1][0]);

    return Vec2{
        mean_.x + cholesky[0][0] * z1,
        mean_.y + cholesky[1][0] * z1 + cholesky[1][1] * z2
    };
}

void NormalDistribution::add_point(Vec2 p) {
    points_.push_back(p);
    calculate_covariance_();
}

NormalDistributionRandom::NormalDistributionRandom(const covariance& cov, Vec2 mean, size_t num_samples)
    : NormalDistribution(cov, mean), num_samples_(num_samples) {}

NormalDistributionRandom::NormalDistributionRandom(std::vector<Vec2> points, size_t num_samples)
    : NormalDistribution(std::move(points)), num_samples_(num_samples) {}

double NormalDistributionRandom::integrate_probability(const Polygon& region) const {
    return integrate_probability(region, Vec2{0.0, 0.0});
}

double NormalDistributionRandom::integrate_probability(const Polygon& region, Vec2 offset) const {
    size_t count = 0;
    for (size_t i = 0; i < num_samples_; ++i) {
        if (region.contains(sample() + offset)) {
            ++count;
        }
    }
    return static_cast<double>(count) / static_cast<double>(num_samples_);
}


double NormalDistributionQuadrature::integrate_probability(const Polygon& region) const {
    return integrate_probability(region, Vec2{0.0, 0.0});
}

double NormalDistributionQuadrature::integrate_probability(const Polygon& region, Vec2 offset) const {
    const auto& verts = region.get_vertices();
    if (verts.size() < 3) return 0.0;

    double poly_area = 0.0;
    for (size_t i = 1; i + 1 < verts.size(); ++i) {
        poly_area += signed_triangle_area(verts[0], verts[i], verts[i + 1]);
    }
    poly_area = std::abs(poly_area);

    double var_measure = cov_[0][0] + cov_[1][1];
    
    // Narrow distribution fallback: if the polygon is much bigger than the variance,
    // the quadrature method breaks. Use a fast sampling strategy instead.
    if (poly_area > 200.0 * var_measure) {
        size_t num_samples = 50; 
        size_t count = 0;
        for (size_t i = 0; i < num_samples; ++i) {
            if (region.contains(sample() + offset)) {
                ++count;
            }
        }
        return static_cast<double>(count) / static_cast<double>(num_samples);
    }

    Vec2 center(0.0, 0.0);
    for (const auto& v : verts) {
        center.x += v.x;
        center.y += v.y;
    }
    center.x /= verts.size();
    center.y /= verts.size();

    double total = 0.0;

    for (size_t i = 0; i < verts.size(); ++i) {
        Vec2 v0 = center;
        Vec2 v1 = verts[i];
        Vec2 v2 = verts[(i + 1) % verts.size()];
        double area = signed_triangle_area(v0, v1, v2);

        for (int q = 0; q < QUAD_NPTS; ++q) {
            Vec2 p = ref_to_physical(v0, v1, v2, quad_r[q], quad_s[q]);
            total += area * quad_w[q] * probability_density(p - offset);
        }
    }
    return std::abs(total);
}
