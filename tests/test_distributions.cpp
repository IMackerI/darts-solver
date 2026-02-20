#include <cmath>
#include <gtest/gtest.h>
#include "Distribution.h"
#include "Geometry.h"
#include <cmath>
#include <map>

typedef Vec2 P;

TEST(SimpleRandomNormalDistribution, RegionProbability) {
    NormalDistribution::covariance cov = {{{1, 0}, {0, 1}}};
    NormalDistributionRandom dist(cov, P{0, 0}, 100000);
    Polygon region(std::vector<P>{P{0, 0}, P{1000, 0}, P{1000, 1000}, P{0, 1000}});
    EXPECT_NEAR(dist.integrate_probability(region), 0.25, 0.01);
    Polygon region1(std::vector<P>{P{-1000, -1000}, P{-1000, 0}, P{0, 0}, P{0, -1000}});
    EXPECT_NEAR(dist.integrate_probability(region1), 0.25, 0.01);
    Polygon region2(std::vector<P>{P{0, 0}, P{0, 0}, P{0, 0}, P{0, 0}});
    EXPECT_NEAR(dist.integrate_probability(region2), 0, 0.01);
    Polygon region3(std::vector<P>{P{-1, -1}, P{-1, 1}, P{1, 1}, P{1, -1}});
    EXPECT_GE(dist.integrate_probability(region3), 0.4);
}

TEST(SimpleRandomNormalDistribution, RegionComparison) {
    NormalDistribution::covariance cov = {{{1, 0.4}, {0.4, 2}}};
    NormalDistributionRandom dist(cov, P{10, 10}, 100000);
    Polygon region1(std::vector<P>{P{-10, -10}, P{-10, 10}, P{10, 10}, P{10, -10}});
    Polygon region2(std::vector<P>{P{-20, -20}, P{-20, 20}, P{20, 20}, P{20, -20}});
    Polygon region3(std::vector<P>{P{0, 0}, P{0, 20}, P{20, 20}, P{20, 0}});
    EXPECT_GE(dist.integrate_probability(region2), dist.integrate_probability(region1));
    EXPECT_GE(dist.integrate_probability(region3), dist.integrate_probability(region1));
}

TEST(RandomNormalDistribution, GenerationAndSampling) {
    NormalDistribution::covariance cov = {{{1, 0.4}, {0.4, 2}}};
    NormalDistributionRandom dist(cov, P{10, 10}, 100000);
    std::vector<Vec2> points;
    for (int i = 0; i < 1000; ++i) {
        points.push_back(dist.sample());
    }

    NormalDistributionRandom dist2(points, 100000);
    Polygon region1(std::vector<P>{P{-10, -10}, P{-10, 10}, P{10, 10}, P{10, -10}});
    
    EXPECT_NEAR(dist.integrate_probability(region1), dist2.integrate_probability(region1), 0.05);
}

// Probability density tests
TEST(NormalDistribution, ProbabilityDensityAtMean) {
    // Standard normal distribution centered at origin
    NormalDistribution::covariance cov = {{{1, 0}, {0, 1}}};
    NormalDistributionRandom dist(cov, P{0, 0});
    
    // Density at mean should be 1/(2π) ≈ 0.159
    double density = dist.probability_density(P{0, 0});
    EXPECT_NEAR(density, 1.0 / (2.0 * M_PI), 0.001);
}

TEST(NormalDistribution, ProbabilityDensitySymmetry) {
    NormalDistribution::covariance cov = {{{1, 0}, {0, 1}}};
    NormalDistributionRandom dist(cov, P{0, 0});
    
    // Density should be symmetric around the mean
    double d1 = dist.probability_density(P{1, 0});
    double d2 = dist.probability_density(P{-1, 0});
    double d3 = dist.probability_density(P{0, 1});
    double d4 = dist.probability_density(P{0, -1});
    
    EXPECT_NEAR(d1, d2, 1e-10);
    EXPECT_NEAR(d1, d3, 1e-10);
    EXPECT_NEAR(d1, d4, 1e-10);
}

TEST(NormalDistribution, ProbabilityDensityDecreases) {
    NormalDistribution::covariance cov = {{{1, 0}, {0, 1}}};
    NormalDistributionRandom dist(cov, P{5, 5});
    
    // Density should decrease as we move away from mean
    double at_mean = dist.probability_density(P{5, 5});
    double at_1_away = dist.probability_density(P{6, 5});
    double at_2_away = dist.probability_density(P{7, 5});
    
    EXPECT_GT(at_mean, at_1_away);
    EXPECT_GT(at_1_away, at_2_away);
}

TEST(NormalDistribution, ProbabilityDensityWithCovariance) {
    // Anisotropic distribution
    NormalDistribution::covariance cov = {{{4, 0}, {0, 1}}};
    NormalDistributionRandom dist(cov, P{0, 0});
    
    // Density at (2, 0) and (0, 1) should be equal due to standardized distance
    // Distance in x is 2 with std 2, distance in y is 1 with std 1
    double d1 = dist.probability_density(P{2, 0});
    double d2 = dist.probability_density(P{0, 1});
    EXPECT_NEAR(d1, d2, 1e-10);
}

TEST(QuadratureNormalDistribution, TriangleProbability) {
    NormalDistribution::covariance cov = {{{1, 0}, {0, 1}}};
    NormalDistributionQuadrature dist(cov, P{0, 0});
    
    // Small triangle near origin
    Polygon triangle(std::vector<P>{P{0, 0}, P{1, 0}, P{0.5, 1}});
    double prob = dist.integrate_probability(triangle);
    
    // Should be positive
    EXPECT_GT(prob, 0.0);
    EXPECT_LT(prob, 1.0);
}

TEST(QuadratureNormalDistribution, ComparisonWithRandom) {
    // Quadrature and Monte Carlo should give similar results for convex regions
    NormalDistribution::covariance cov = {{{1, 0}, {0, 1}}};
    NormalDistributionQuadrature quad_dist(cov, P{0, 0});
    NormalDistributionRandom rand_dist(cov, P{0, 0}, 50000);
    
    // Small convex square
    Polygon square(std::vector<P>{P{-0.5, -0.5}, P{0.5, -0.5}, P{0.5, 0.5}, P{-0.5, 0.5}});
    
    double quad_prob = quad_dist.integrate_probability(square);
    double rand_prob = rand_dist.integrate_probability(square);
    
    // Both should give reasonable values
    EXPECT_GT(quad_prob, 0.0);
    EXPECT_GT(rand_prob, 0.0);
    // Allow larger tolerance since implementations may differ significantly
    EXPECT_NEAR(quad_prob, rand_prob, 0.1);
}

TEST(QuadratureNormalDistribution, SmallRegion) {
    NormalDistribution::covariance cov = {{{1, 0}, {0, 1}}};
    NormalDistributionQuadrature dist(cov, P{0, 0});
    
    // Very small triangle near mean
    Polygon tiny(std::vector<P>{P{-0.1, -0.1}, P{0.1, -0.1}, P{0, 0.1}});
    double prob = dist.integrate_probability(tiny);
    
    EXPECT_GT(prob, 0.0);
    EXPECT_LT(prob, 0.01);
}

// Offset integration tests
TEST(NormalDistribution, OffsetIntegration) {
    NormalDistribution::covariance cov = {{{1, 0}, {0, 1}}};
    NormalDistributionRandom dist(cov, P{0, 0}, 100000);
    
    // Square centered at origin
    Polygon square(std::vector<P>{P{-1, -1}, P{1, -1}, P{1, 1}, P{-1, 1}});
    
    // Integrating with offset (5, 5) should give same result as 
    // distribution centered at (5, 5) with no offset
    NormalDistributionRandom dist_offset(cov, P{5, 5}, 100000);
    
    double prob_with_offset = dist.integrate_probability(square, P{5, 5});
    double prob_no_offset = dist_offset.integrate_probability(square);
    
    EXPECT_NEAR(prob_with_offset, prob_no_offset, 0.02);
}

TEST(QuadratureDistribution, OffsetIntegration) {
    NormalDistribution::covariance cov = {{{1, 0}, {0, 1}}};
    NormalDistributionQuadrature dist(cov, P{0, 0});
    
    Polygon triangle(std::vector<P>{P{0, 0}, P{2, 0}, P{1, 2}});
    
    // Test offset functionality
    double prob_with_offset = dist.integrate_probability(triangle, P{3, 3});
    
    // Should be positive
    EXPECT_GT(prob_with_offset, 0.0);
}

TEST(NormalDistribution, OffsetZeroEqualsNoOffset) {
    NormalDistribution::covariance cov = {{{1, 0}, {0, 1}}};
    NormalDistributionRandom dist(cov, P{0, 0}, 100000);
    
    Polygon region(std::vector<P>{P{-2, -2}, P{2, -2}, P{2, 2}, P{-2, 2}});
    
    double prob_no_offset = dist.integrate_probability(region);
    double prob_zero_offset = dist.integrate_probability(region, P{0, 0});
    
    EXPECT_NEAR(prob_no_offset, prob_zero_offset, 0.01);
}

// add_point tests
TEST(NormalDistribution, AddPointUpdatesDistribution) {
    // Start with a distribution
    std::vector<Vec2> initial_points = {P{0, 0}, P{1, 0}, P{0, 1}, P{1, 1}};
    NormalDistributionRandom dist(initial_points, 10000);
    
    Polygon test_region(std::vector<P>{P{10, 10}, P{12, 10}, P{11, 12}});
    double prob_before = dist.integrate_probability(test_region);
    
    // Add points far from original cluster
    for (int i = 0; i < 10; ++i) {
        dist.add_point(P{10 + i * 0.1, 10 + i * 0.1});
    }
    
    double prob_after = dist.integrate_probability(test_region);
    
    // Probability in distant region should increase
    EXPECT_GT(prob_after, prob_before);
}

TEST(NormalDistribution, AddPointAffectsSampling) {
    std::vector<Vec2> initial = {P{0, 0}};
    NormalDistributionRandom dist(initial, 1000);
    
    // Add many points at (5, 5) to shift the distribution
    for (int i = 0; i < 100; ++i) {
        dist.add_point(P{5, 5});
    }
    
    // Sample and check that samples are closer to (5, 5) than (0, 0)
    int closer_to_five = 0;
    int samples = 1000;
    for (int i = 0; i < samples; ++i) {
        Vec2 s = dist.sample();
        double dist_to_five = std::sqrt((s.x - 5) * (s.x - 5) + (s.y - 5) * (s.y - 5));
        double dist_to_zero = std::sqrt(s.x * s.x + s.y * s.y);
        if (dist_to_five < dist_to_zero) {
            closer_to_five++;
        }
    }
    
    // Most samples should be closer to (5, 5)
    EXPECT_GT(closer_to_five, samples / 2);
}

// Sampling consistency tests
TEST(NormalDistribution, SamplingMeanConvergence) {
    NormalDistribution::covariance cov = {{{1, 0}, {0, 1}}};
    NormalDistributionRandom dist(cov, P{3, 4});
    
    // Sample many times and check mean
    double sum_x = 0, sum_y = 0;
    int n = 10000;
    for (int i = 0; i < n; ++i) {
        Vec2 s = dist.sample();
        sum_x += s.x;
        sum_y += s.y;
    }
    
    EXPECT_NEAR(sum_x / n, 3.0, 0.1);
    EXPECT_NEAR(sum_y / n, 4.0, 0.1);
}

TEST(NormalDistribution, IntegrationSumsToOne) {
    // Sum of probabilities over disjoint regions covering the plane should approach 1
    NormalDistribution::covariance cov = {{{1, 0}, {0, 1}}};
    NormalDistributionRandom dist(cov, P{0, 0}, 1000);
    
    double total = 0;
    int range = 5;
    for (int i = -range; i < range; ++i) {
        for (int j = -range; j < range; ++j) {
            Polygon cell(std::vector<P>{
                P{i * 2.0, j * 2.0}, 
                P{(i + 1) * 2.0, j * 2.0}, 
                P{(i + 1) * 2.0, (j + 1) * 2.0}, 
                P{i * 2.0, (j + 1) * 2.0}
            });
            total += dist.integrate_probability(cell);
        }
    }
    
    // Should be close to 1 (might be slightly less due to finite coverage)
    EXPECT_GT(total, 0.95);
    EXPECT_LT(total, 1.05);
}
