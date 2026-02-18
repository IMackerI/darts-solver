#include <gtest/gtest.h>
#include "Game.h"
#include "Distribution.h"
#include "Geometry.h"
#include <sstream>
#include <map>

typedef Vec2 P;

// HitData tests
TEST(HitData, Construction) {
    HitData hit(HitData::Type::DOUBLE, -20);
    EXPECT_EQ(hit.type, HitData::Type::DOUBLE);
    EXPECT_EQ(hit.diff, -20);
}

TEST(HitData, DefaultConstruction) {
    HitData hit;
    // Should be constructible with defaults
}

TEST(HitData, Comparison) {
    HitData h1(HitData::Type::NORMAL, -10);
    HitData h2(HitData::Type::DOUBLE, -10);
    HitData h3(HitData::Type::NORMAL, -20);
    
    // Type comparison takes precedence
    EXPECT_TRUE(h1 < h2);
    
    // Same type, compare by diff (more negative is smaller)
    EXPECT_TRUE(h3 < h1);  // -20 < -10
}

TEST(HitData, Types) {
    HitData normal(HitData::Type::NORMAL, -5);
    HitData double_hit(HitData::Type::DOUBLE, -10);
    HitData treble(HitData::Type::TREBLE, -15);
    
    EXPECT_TRUE(normal.type == HitData::Type::NORMAL);
    EXPECT_TRUE(double_hit.type == HitData::Type::DOUBLE);
    EXPECT_TRUE(treble.type == HitData::Type::TREBLE);
}

// Target tests using input streams
TEST(Target, LoadFromStream) {
    // Create a simple target definition
    // Format: score, num_points, color, type, then vertices
    std::stringstream input;
    input << "1\n";  // 1 bed
    input << "20\n";  // Score (positive)
    input << "4\n";  // 4 vertices
    input << "red\n";  // Color (ignored)
    input << "normal\n";  // Type
    input << "0 0\n1 0\n1 1\n0 1\n";  // Square vertices
    
    Target target(input);
    
    // Test after_hit
    HitData hit = target.after_hit(P{0.5, 0.5});
    EXPECT_EQ(hit.diff, -20);
    EXPECT_EQ(hit.type, HitData::Type::NORMAL);
    
    // Miss
    HitData miss = target.after_hit(P{5, 5});
    EXPECT_EQ(miss.diff, 0);
}

TEST(Target, MultipleBeds) {
    std::stringstream input;
    input << "2\n";  // 2 beds
    
    // First bed - square at origin
    input << "10\n";  // Score
    input << "4\n";   // Vertices
    input << "red\n"; // Color
    input << "normal\n"; // Type
    input << "0 0\n2 0\n2 2\n0 2\n";
    
    // Second bed - square offset
    input << "20\n";  // Score
    input << "4\n";   // Vertices
    input << "blue\n"; // Color
    input << "double\n"; // Type
    input << "5 5\n7 5\n7 7\n5 7\n";
    
    Target target(input);
    
    HitData hit1 = target.after_hit(P{1, 1});
    EXPECT_EQ(hit1.diff, -10);
    EXPECT_EQ(hit1.type, HitData::Type::NORMAL);
    
    HitData hit2 = target.after_hit(P{6, 6});
    EXPECT_EQ(hit2.diff, -20);
    EXPECT_EQ(hit2.type, HitData::Type::DOUBLE);
}

TEST(Target, TrebleType) {
    std::stringstream input;
    input << "1\n";
    input << "15\n";  // Score
    input << "3\n";  // Triangle vertices
    input << "green\n";  // Color
    input << "treble\n";  // Type
    input << "0 0\n3 0\n1.5 3\n";
    
    Target target(input);
    
    HitData hit = target.after_hit(P{1.5, 1});
    EXPECT_EQ(hit.type, HitData::Type::TREBLE);
    EXPECT_EQ(hit.diff, -15);
}

TEST(Target, GetBeds) {
    std::stringstream input;
    input << "2\n";
    input << "5\n4\nwhite\nnormal\n0 0\n1 0\n1 1\n0 1\n";
    input << "10\n4\nblack\ndouble\n2 2\n3 2\n3 3\n2 3\n";
    
    Target target(input);
    const auto& beds = target.get_beds();
    
    EXPECT_EQ(beds.size(), 2);
}

TEST(Target, Import) {
    std::stringstream input;
    input << "1\n20\n4\nred\nnormal\n0 0\n1 0\n1 1\n0 1\n";
    
    Target target;
    target.import(input);
    
    HitData hit = target.after_hit(P{0.5, 0.5});
    EXPECT_EQ(hit.diff, -20);
}

// Game bounds tests
TEST(Game, GetTargetBounds) {
    std::stringstream input;
    input << "1\n20\n4\nred\nnormal\n0 0\n10 0\n10 10\n0 10\n";
    Target target(input);
    
    NormalDistribution::covariance cov = {{{1, 0}, {0, 1}}};
    NormalDistributionRandom dist(cov, P{0, 0});
    
    GameFinishOnAny game(target, dist);
    
    auto [min_corner, max_corner] = game.get_target_bounds();
    
    // Bounds should include the target with padding
    EXPECT_LT(min_corner.x, 0);
    EXPECT_LT(min_corner.y, 0);
    EXPECT_GT(max_corner.x, 10);
    EXPECT_GT(max_corner.y, 10);
}

// GameFinishOnAny tests
TEST(GameFinishOnAny, WinStateTransition) {
    // Create simple target with one scoring bed
    std::stringstream input;
    input << "1\n20\n4\nred\nnormal\n-5 -5\n5 -5\n5 5\n-5 5\n";
    Target target(input);
    
    // Perfect distribution centered at origin (always hits the bed)
    NormalDistribution::covariance cov = {{{0.001, 0}, {0, 0.001}}};
    NormalDistributionRandom dist(cov, P{0, 0}, 1000);
    
    GameFinishOnAny game(target, dist);
    
    // Throw from state 20 - should result in state 0 (win)
    auto outcomes = game.throw_at(P{0, 0}, 20);
    
    // Should have at least one outcome
    EXPECT_GT(outcomes.size(), 0);
    
    // Check that state 0 appears in outcomes with high probability
    bool found_win = false;
    for (const auto& [state, prob] : outcomes) {
        if (state == 0) {
            found_win = true;
            EXPECT_GT(prob, 0.8);  // Should be high probability
        }
    }
    EXPECT_TRUE(found_win);
}

TEST(GameFinishOnAny, BustDoesNotChange) {
    // Hitting more than remaining score should not change state (bust)
    std::stringstream input;
    input << "1\n50\n4\nred\nnormal\n-5 -5\n5 -5\n5 5\n-5 5\n";
    Target target(input);
    
    NormalDistribution::covariance cov = {{{0.001, 0}, {0, 0.001}}};
    NormalDistributionRandom dist(cov, P{0, 0}, 1000);
    
    GameFinishOnAny game(target, dist);
    
    // Throw from state 30 - hitting -50 should bust
    auto outcomes = game.throw_at(P{0, 0}, 30);
    
    bool found_bust = false;
    for (const auto& [state, prob] : outcomes) {
        if (state == 30) {  // State unchanged
            found_bust = true;
        }
    }
    EXPECT_TRUE(found_bust);
}

TEST(GameFinishOnAny, MissDoesNotChange) {
    // Missing the target should not change state
    std::stringstream input;
    input << "1\n20\n4\nred\nnormal\n-1 -1\n1 -1\n1 1\n-1 1\n";
    Target target(input);
    
    // Wide distribution - many misses
    NormalDistribution::covariance cov = {{{100, 0}, {0, 100}}};
    NormalDistributionRandom dist(cov, P{50, 50}, 10000);  // Aim far from target
    
    GameFinishOnAny game(target, dist);
    
    auto outcomes = game.throw_at(P{50, 50}, 100);
    
    // Should have outcome where state remains 100
    bool found_unchanged = false;
    for (const auto& [state, prob] : outcomes) {
        if (state == 100) {
            found_unchanged = true;
        }
    }
    EXPECT_TRUE(found_unchanged);
}

TEST(GameFinishOnAny, PartialProgress) {
    std::stringstream input;
    input << "1\n20\n4\nred\nnormal\n-5 -5\n5 -5\n5 5\n-5 5\n";
    Target target(input);
    
    NormalDistribution::covariance cov = {{{0.001, 0}, {0, 0.001}}};
    NormalDistributionRandom dist(cov, P{0, 0}, 1000);
    
    GameFinishOnAny game(target, dist);
    
    // From state 100, hitting -20 should go to 80
    auto outcomes = game.throw_at(P{0, 0}, 100);
    
    bool found_80 = false;
    for (const auto& [state, prob] : outcomes) {
        if (state == 80) {
            found_80 = true;
            EXPECT_GT(prob, 0.8);
        }
    }
    EXPECT_TRUE(found_80);
}

// GameFinishOnDouble tests
TEST(GameFinishOnDouble, MustFinishOnDouble) {
    // Create target with double and normal beds
    std::stringstream input;
    input << "2\n";
    input << "20\n4\nred\ndouble\n-2 -2\n2 -2\n2 2\n-2 2\n";  // DOUBLE
    input << "20\n4\nblue\nnormal\n5 5\n7 5\n7 7\n5 7\n";  // NORMAL
    
    Target target(input);
    
    NormalDistribution::covariance cov = {{{0.001, 0}, {0, 0.001}}};
    
    // Test double finish - should win
    NormalDistributionRandom dist_double(cov, P{0, 0}, 1000);
    GameFinishOnDouble game(target, dist_double);
    
    auto outcomes_double = game.throw_at(P{0, 0}, 20);
    bool found_win = false;
    for (const auto& [state, prob] : outcomes_double) {
        if (state == 0) {
            found_win = true;
        }
    }
    EXPECT_TRUE(found_win);
    
    // Test normal finish - should bust
    NormalDistributionRandom dist_normal(cov, P{6, 6}, 1000);
    GameFinishOnDouble game2(target, dist_normal);
    
    auto outcomes_normal = game2.throw_at(P{6, 6}, 20);
    bool stayed_at_20 = false;
    for (const auto& [state, prob] : outcomes_normal) {
        if (state == 20) {
            stayed_at_20 = true;
        }
    }
    EXPECT_TRUE(stayed_at_20);
}

TEST(GameFinishOnDouble, StateOneIsBust) {
    // Reaching state 1 should bust (can't finish on double)
    std::stringstream input;
    input << "1\n1\n4\nred\ndouble\n-5 -5\n5 -5\n5 5\n-5 5\n";  // DOUBLE scoring 1
    Target target(input);
    
    NormalDistribution::covariance cov = {{{0.001, 0}, {0, 0.001}}};
    NormalDistributionRandom dist(cov, P{0, 0}, 1000);
    
    GameFinishOnDouble game(target, dist);
    
    // From state 2, hitting -1 should bust (would go to 1)
    auto outcomes = game.throw_at(P{0, 0}, 2);
    
    bool stayed_at_2 = false;
    for (const auto& [state, prob] : outcomes) {
        if (state == 2) {
            stayed_at_2 = true;
        }
    }
    EXPECT_TRUE(stayed_at_2);
}

TEST(GameFinishOnDouble, NormalProgressWorks) {
    // Normal hits should progress when not finishing
    std::stringstream input;
    input << "1\n20\n4\nred\nnormal\n-5 -5\n5 -5\n5 5\n-5 5\n";  // NORMAL
    Target target(input);
    
    NormalDistribution::covariance cov = {{{0.001, 0}, {0, 0.001}}};
    NormalDistributionRandom dist(cov, P{0, 0}, 1000);
    
    GameFinishOnDouble game(target, dist);
    
    // From state 100, hitting -20 should go to 80
    auto outcomes = game.throw_at(P{0, 0}, 100);
    
    bool found_80 = false;
    for (const auto& [state, prob] : outcomes) {
        if (state == 80) {
            found_80 = true;
            EXPECT_GT(prob, 0.8);
        }
    }
    EXPECT_TRUE(found_80);
}

// Probability distribution tests
TEST(Game, ThrowAtProbabilitiesSum) {
    // Probabilities should sum to approximately 1
    std::stringstream input;
    input << "2\n";
    input << "10\n4\nred\nnormal\n-3 -3\n3 -3\n3 3\n-3 3\n";
    input << "5\n4\nblue\nnormal\n5 5\n8 5\n8 8\n5 8\n";
    Target target(input);
    
    NormalDistribution::covariance cov = {{{2, 0}, {0, 2}}};
    NormalDistributionRandom dist(cov, P{0, 0}, 5000);
    
    GameFinishOnAny game(target, dist);
    
    auto outcomes = game.throw_at(P{0, 0}, 50);
    
    double total_prob = 0;
    for (const auto& [state, prob] : outcomes) {
        total_prob += prob;
        EXPECT_GE(prob, 0);  // No negative probabilities
        EXPECT_LE(prob, 1);  // No probability > 1
    }
    
    EXPECT_NEAR(total_prob, 1.0, 0.01);
}

TEST(Game, SampleConsistentWithDistribution) {
    // Sampling many times should give distribution consistent with throw_at
    std::stringstream input;
    input << "1\n20\n4\nred\nnormal\n-5 -5\n5 -5\n5 5\n-5 5\n";
    Target target(input);
    
    NormalDistribution::covariance cov = {{{1, 0}, {0, 1}}};
    NormalDistributionRandom dist(cov, P{0, 0}, 5000);
    
    GameFinishOnAny game(target, dist);
    
    // Get theoretical distribution
    auto theoretical = game.throw_at(P{0, 0}, 100);
    
    // Sample many times
    std::map<unsigned int, int> sampled_counts;
    int num_samples = 1000;
    for (int i = 0; i < num_samples; ++i) {
        unsigned int result = game.throw_at_sample(P{0, 0}, 100);
        sampled_counts[result]++;
    }
    
    // Check that major outcomes appear in samples
    for (const auto& [state, prob] : theoretical) {
        if (prob > 0.1) {  // Only check significant outcomes
            EXPECT_GT(sampled_counts[state], 0);
        }
    }
}

TEST(Game, DifferentAimPoints) {
    std::stringstream input;
    input << "2\n";
    input << "20\n4\nred\nnormal\n-5 -5\n5 -5\n5 5\n-5 5\n";  // At origin
    input << "10\n4\nblue\nnormal\n10 10\n15 10\n15 15\n10 15\n";  // Offset
    Target target(input);
    
    NormalDistribution::covariance cov = {{{0.1, 0}, {0, 0.1}}};
    NormalDistributionRandom dist(cov, P{0, 0}, 5000);
    
    GameFinishOnAny game(target, dist);
    
    // Aiming at origin should mostly hit -20
    auto outcomes1 = game.throw_at(P{0, 0}, 100);
    bool found_80 = false;
    for (const auto& [state, prob] : outcomes1) {
        if (state == 80) {
            found_80 = true;
        }
    }
    EXPECT_TRUE(found_80);
    
    // Aiming at offset should mostly hit -10
    auto outcomes2 = game.throw_at(P{12.5, 12.5}, 100);
    bool found_90 = false;
    for (const auto& [state, prob] : outcomes2) {
        if (state == 90) {
            found_90 = true;
        }
    }
    EXPECT_TRUE(found_90);
}

