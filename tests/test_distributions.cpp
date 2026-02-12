#include <gtest/gtest.h>
#include "Distribution.h"
#include "Geometry.h"

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
