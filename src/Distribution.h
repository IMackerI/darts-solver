#ifndef DISTRIBUTION_HEADER
#define DISTRIBUTION_HEADER

#include "Geometry.h"

#include <cstddef>
#include <vector>
#include <array>

/**
 * @defgroup distributions Probability Distributions
 * @brief 2D probability distributions for modeling dart throws.
 */

/**
 * @brief Abstract base class for 2D probability distributions.
 * @ingroup distributions
 * 
 * Provides interface for sampling, density evaluation, and integration
 * over polygonal regions.
 */
class Distribution {
protected:
    std::vector<Vec2> points_;

public:
    Distribution() = default;
    Distribution(std::vector<Vec2> points) : points_(std::move(points)) {}
    virtual ~Distribution() = default;
    
    /**
     * @brief Evaluate probability density function at point p.
     * @param p Point to evaluate
     * @return Probability density value
     */
    [[nodiscard]] virtual double probability_density(Vec2 p) const = 0;
    
    /**
     * @brief Generate a random sample from the distribution.
     * @return Sampled point
     */
    [[nodiscard]] virtual Vec2 sample() const = 0;
    
    /**
     * @brief Integrate probability over a polygonal region.
     * @param region Polygon to integrate over
     * @return Total probability mass in region (between 0 and 1)
     */
    [[nodiscard]] virtual double integrate_probability(const Polygon& region) const = 0;
    
    /**
     * @brief Integrate probability over a translated polygonal region.
     * @param region Polygon to integrate over
     * @param offset Translation to apply to distribution
     * @return Total probability mass in region (between 0 and 1)
     */
    [[nodiscard]] virtual double integrate_probability(const Polygon& region, Vec2 offset) const = 0;
    
    /**
     * @brief Add a data point and recompute distribution parameters.
     * @param p Point to add
     */
    virtual void add_point(Vec2 p) = 0;
};

/**
 * @brief 2D Normal (Gaussian) distribution with mean and covariance.
 * @ingroup distributions
 *
 * Represents a bivariate normal distribution. Can be constructed from
 * explicit parameters or fitted to sample points.
 *
 * Example usage:
 * @code
 * // From covariance matrix
 * NormalDistribution::covariance cov = {{{1.0, 0.0}, {0.0, 1.0}}};
 * auto dist = NormalDistributionRandom(cov, Vec2{0, 0});
 * 
 * // From sample points
 * std::vector<Vec2> samples = {{1.2, 0.5}, {0.8, -0.3}, {1.5, 0.2}};
 * auto dist2 = NormalDistributionRandom(samples);
 * @endcode
 */
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
    
    /**
     * @brief Construct from explicit covariance matrix and mean.
     * @param cov 2x2 covariance matrix
     * @param mean Mean vector (default is origin)
     */
    NormalDistribution(const covariance& cov, Vec2 mean = Vec2{0, 0});
    
    /**
     * @brief Construct by fitting to sample points.
     * Computes sample mean and covariance from provided points.
     * @param points Sample points to fit distribution to
     */
    NormalDistribution(std::vector<Vec2> points);
    
    /**
     * @brief Evaluate 2D Gaussian PDF at point p.
     * Uses formula: f(x) = (1/(2π√|Σ|)) * exp(-0.5(x-μ)ᵀΣ⁻¹(x-μ))
     */
    [[nodiscard]] double probability_density(Vec2 p) const override;
    
    /**
     * @brief Sample from 2D normal distribution using Cholesky decomposition.
     */
    [[nodiscard]] Vec2 sample() const override;
    
    [[nodiscard]] virtual double integrate_probability(const Polygon& region) const override = 0;
    [[nodiscard]] virtual double integrate_probability(const Polygon& region, Vec2 offset) const override = 0;
    
    void add_point(Vec2 p) override;
};

/**
 * @ingroup distributions
 * @brief Normal distribution using Monte Carlo integration.
 *
 * Integration over regions is performed by random sampling.
 * Faster for complex regions but approximate. Accuracy improves with more samples.
 *
 * Example usage:
 * @code
 * NormalDistribution::covariance cov = {{{1.0, 0.0}, {0.0, 1.0}}};
 * auto dist = NormalDistributionRandom(cov, Vec2{0, 0}, 50000); // 50k samples
 * 
 * Polygon target({Vec2{-1,-1}, Vec2{1,-1}, Vec2{1,1}, Vec2{-1,1}});
 * double prob = dist.integrate_probability(target); // ≈ P(dart in target)
 * @endcode
 */
class NormalDistributionRandom final : public NormalDistribution {
private:
    size_t num_samples_;
public:
    /**
     * @brief Construct from covariance and mean.
     * @param cov Covariance matrix
     * @param mean Mean vector
     * @param num_samples Number of Monte Carlo samples for integration (default 10000)
     */
    NormalDistributionRandom(const covariance& cov, Vec2 mean = Vec2{0, 0}, size_t num_samples = 10000);
    
    /**
     * @brief Construct by fitting to points.
     * @param points Sample points
     * @param num_samples Number of Monte Carlo samples for integration (default 1000)
     */
    NormalDistributionRandom(std::vector<Vec2> points, size_t num_samples = 1000);
    
    /**
     * @brief Monte Carlo integration over region.
     * Samples random points and counts how many fall inside the polygon.
     */
    [[nodiscard]] double integrate_probability(const Polygon& region) const override;
    
    /**
     * @brief Monte Carlo integration over translated region.
     */
    [[nodiscard]] double integrate_probability(const Polygon& region, Vec2 offset) const override;
    
    /**
     * @brief Adjust number of samples for integration.
     * Higher values increase accuracy but slow down computation.
     * @param num_samples Number of Monte Carlo samples to use
     */
    void set_integration_precision(size_t num_samples) {
        num_samples_ = num_samples;
    }
};

/**
 * @ingroup distributions
 * @brief Normal distribution using Gauss quadrature integration.
 *
 * Integration uses Dunavant's 7-point degree-5 quadrature rule on triangles.
 * Exact for polynomials up to degree 5. Much more accurate than Monte Carlo
 * for smooth distributions but requires convex polygons.
 *
 * @note Assumes polygons are convex (uses fan triangulation).
 *
 * Example usage:
 * @code
 * NormalDistribution::covariance cov = {{{1.0, 0.0}, {0.0, 1.0}}};
 * auto dist = NormalDistributionQuadrature(cov);
 * 
 * Polygon triangle({Vec2{0,0}, Vec2{2,0}, Vec2{1,1.5}});
 * double prob = dist.integrate_probability(triangle); // High precision
 * @endcode
 */
class NormalDistributionQuadrature final : public NormalDistribution {
    using NormalDistribution::NormalDistribution;
    
public:
    /**
     * @brief Gauss quadrature integration over convex polygon.
     * Triangulates polygon from first vertex and applies 7-point quadrature.
     */
    [[nodiscard]] double integrate_probability(const Polygon& region) const override;
    
    /**
     * @brief Gauss quadrature integration over translated convex polygon.
     */
    [[nodiscard]] double integrate_probability(const Polygon& region, Vec2 offset) const override;
};

#endif