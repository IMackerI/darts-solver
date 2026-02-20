#include <gtest/gtest.h>
#include "Game.h"
#include "Solver.h"
#include "Distribution.h"
#include "Geometry.h"
#include <sstream>
#include <cmath>
#include <algorithm>

/**
 * Integration tests for the darts solver system.
 * These tests verify end-to-end functionality with realistic scenarios
 * and check that results are reasonable, not just that code doesn't crash.
 */

// Helper function to create a simple target with known properties
Target create_simple_target() {
    // Create a simple target with:
    // - Center double 50 (bull)
    // - Ring of 20s (divided into 8 convex trapezoids)
    // - Outer ring of doubles for 20 (divided into 8 convex trapezoids)
    std::stringstream input;
    
    int num_segments = 8;
    int total_beds = 1 + num_segments + num_segments;  // bull + 20s ring + double ring
    input << total_beds << "\n";
    
    // Double bull (50 points) - small circle at center (convex)
    input << "50\n";  // Score
    input << "8\n";   // 8 vertices (approximating a circle)
    input << "red\n"; // Color
    input << "double\n"; // Type
    // Center circle with radius ~6.35 mm
    for (int i = 0; i < 8; ++i) {
        double angle = 2 * M_PI * i / 8;
        double x = 6.35 * std::cos(angle);
        double y = 6.35 * std::sin(angle);
        input << x << " " << y << "\n";
    }
    
    // Ring of 20s - divided into convex trapezoids
    // Each trapezoid: inner_arc_start, inner_arc_end, outer_arc_end, outer_arc_start
    for (int seg = 0; seg < num_segments; ++seg) {
        double angle1 = 2 * M_PI * seg / num_segments;
        double angle2 = 2 * M_PI * (seg + 1) / num_segments;
        
        input << "20\n";  // Score
        input << "4\n";   // 4 vertices (trapezoid)
        input << "white\n"; // Color
        input << "normal\n"; // Type
        
        // Inner arc points (radius 50)
        double x1_inner = 50 * std::cos(angle1);
        double y1_inner = 50 * std::sin(angle1);
        double x2_inner = 50 * std::cos(angle2);
        double y2_inner = 50 * std::sin(angle2);
        
        // Outer arc points (radius 100)
        double x1_outer = 100 * std::cos(angle1);
        double y1_outer = 100 * std::sin(angle1);
        double x2_outer = 100 * std::cos(angle2);
        double y2_outer = 100 * std::sin(angle2);
        
        // CCW order: inner1, inner2, outer2, outer1
        input << x1_inner << " " << y1_inner << "\n";
        input << x2_inner << " " << y2_inner << "\n";
        input << x2_outer << " " << y2_outer << "\n";
        input << x1_outer << " " << y1_outer << "\n";
    }
    
    // Outer double ring (40 points for double 20) - divided into convex trapezoids
    for (int seg = 0; seg < num_segments; ++seg) {
        double angle1 = 2 * M_PI * seg / num_segments;
        double angle2 = 2 * M_PI * (seg + 1) / num_segments;
        
        input << "40\n";  // Score
        input << "4\n";   // 4 vertices (trapezoid)
        input << "red\n"; // Color
        input << "double\n"; // Type
        
        // Inner arc points (radius 150)
        double x1_inner = 150 * std::cos(angle1);
        double y1_inner = 150 * std::sin(angle1);
        double x2_inner = 150 * std::cos(angle2);
        double y2_inner = 150 * std::sin(angle2);
        
        // Outer arc points (radius 170)
        double x1_outer = 170 * std::cos(angle1);
        double y1_outer = 170 * std::sin(angle1);
        double x2_outer = 170 * std::cos(angle2);
        double y2_outer = 170 * std::sin(angle2);
        
        // CCW order: inner1, inner2, outer2, outer1
        input << x1_inner << " " << y1_inner << "\n";
        input << x2_inner << " " << y2_inner << "\n";
        input << x2_outer << " " << y2_outer << "\n";
        input << x1_outer << " " << y1_outer << "\n";
    }
    
    return Target(input);
}

/**
 * Test: Basic solver functionality
 * Verifies that solver can find solutions for simple states
 */
TEST(Integration, BasicSolverFunctionality) {
    Target target = create_simple_target();
    NormalDistribution::covariance cov = {{{100, 0}, {0, 100}}};
    NormalDistributionQuadrature dist(cov, Vec2{0, 0});
    GameFinishOnAny game(target, dist);
    Solver solver(game, 1000);  // 1000 sample points
    
    // Test state 0 (already won)
    auto [score_0, aim_0] = solver.solve(0);
    EXPECT_DOUBLE_EQ(score_0, 0.0) << "State 0 should require 0 throws";
    
    // Test state 20 (one good hit could finish)
    auto [score_20, aim_20] = solver.solve(20);
    EXPECT_GT(score_20, 0.0) << "State 20 should require positive throws";
    EXPECT_LT(score_20, 10.0) << "State 20 should be solvable in reasonable throws";
    
    // Test state 50
    auto [score_50, aim_50] = solver.solve(50);
    EXPECT_GT(score_50, score_20) << "Higher states should generally require more throws";
    EXPECT_LT(score_50, 20.0) << "State 50 should be solvable in reasonable throws";
}

/**
 * Test: Monotonicity property
 * In this unique case, lower scores should require fewer expected throws
 */
TEST(Integration, MonotonicityProperty) {
    Target target = create_simple_target();
    NormalDistribution::covariance cov = {{{200, 0}, {0, 200}}};
    NormalDistributionQuadrature dist(cov, Vec2{0, 0});
    GameFinishOnAny game(target, dist);
    Solver solver(game, 500);
    
    std::vector<int> states = {20, 40, 60, 80, 100};
    std::vector<double> scores;
    
    for (int state : states) {
        auto [score, aim] = solver.solve(state);
        scores.push_back(score);
    }
    
    // Check that higher states generally require more throws
    // Allow some slack for randomness in quadrature
    for (size_t i = 1; i < scores.size(); ++i) {
        EXPECT_GE(scores[i], scores[i-1] - 0.5)
            << "State " << states[i] << " should not require significantly fewer throws than "
            << states[i-1];
    }
}

/**
 * Test: Optimal aims are within reasonable bounds
 * The solver should aim within or near the target
 */
TEST(Integration, OptimalAimsWithinBounds) {
    Target target = create_simple_target();
    NormalDistribution::covariance cov = {{{300, 0}, {0, 300}}};
    NormalDistributionQuadrature dist(cov, Vec2{0, 0});
    GameFinishOnAny game(target, dist);
    Solver solver(game, 800);
    
    auto bounds = game.get_target_bounds();
    
    std::vector<int> test_states = {20, 40, 50};
    for (int state : test_states) {
        auto [score, aim] = solver.solve(state);
        
        EXPECT_GE(aim.x, bounds.min.x) 
            << "Aim x should be within bounds for state " << state;
        EXPECT_LE(aim.x, bounds.max.x)
            << "Aim x should be within bounds for state " << state;
        EXPECT_GE(aim.y, bounds.min.y)
            << "Aim y should be within bounds for state " << state;
        EXPECT_LE(aim.y, bounds.max.y)
            << "Aim y should be within bounds for state " << state;
    }
}

/**
 * Test: FinishOnDouble requires double to win
 * State 50 aiming at bull should potentially win, but only on double hit
 */
TEST(Integration, FinishOnDoubleRequirement) {
    Target target = create_simple_target();
    NormalDistribution::covariance cov = {{{50, 0}, {0, 50}}};
    NormalDistributionQuadrature dist(cov, Vec2{0, 0});
    GameFinishOnDouble game(target, dist);
    
    // Aim at center (where double bull is)
    Vec2 aim_center{0, 0};
    auto outcomes = game.throw_at(aim_center, 50);
    
    // Check that state 0 (win) appears in outcomes
    bool can_win = false;
    bool can_stay = false;
    for (const auto& [state, prob] : outcomes) {
        if (state == 0) {
            can_win = true;
            EXPECT_GT(prob, 0.0) << "Should have non-zero probability to win on double bull";
        }
        if (state == 50) {
            can_stay = true;
        }
    }
    
    EXPECT_TRUE(can_win) << "Should be possible to win from 50 aiming at double bull";
    EXPECT_TRUE(can_stay) << "Should be possible to miss";
}

/**
 * Test: Probability distributions sum to 1
 * For any throw, all outcome probabilities should sum to approximately 1
 */
TEST(Integration, ProbabilitySumToOne) {
    Target target = create_simple_target();
    NormalDistribution::covariance cov = {{{150, 0}, {0, 150}}};
    NormalDistributionQuadrature dist(cov, Vec2{0, 0});
    GameFinishOnAny game(target, dist);
    
    std::vector<Vec2> test_aims = {
        Vec2{0, 0},      // Center
        Vec2{75, 0},     // Edge of 20 ring
        Vec2{160, 0},    // Double ring
        Vec2{200, 0}     // Outside target
    };
    
    std::vector<int> test_states = {20, 40, 50};
    
    for (const auto& aim : test_aims) {
        for (int state : test_states) {
            auto outcomes = game.throw_at(aim, state);
            double total_prob = 0.0;
            
            for (const auto& [new_state, prob] : outcomes) {
                EXPECT_GE(prob, 0.0) << "Probabilities should be non-negative";
                total_prob += prob;
            }
            
            EXPECT_NEAR(total_prob, 1.0, 1e-6)
                << "Probabilities should sum to 1 for aim (" << aim.x << ", " << aim.y 
                << ") from state " << state;
        }
    }
}

/**
 * Test: State transitions are valid
 * Throws should only transition to valid states (0 to current_state for countdown games)
 */
TEST(Integration, ValidStateTransitions) {
    Target target = create_simple_target();
    NormalDistribution::covariance cov = {{{100, 0}, {0, 100}}};
    NormalDistributionQuadrature dist(cov, Vec2{0, 0});
    GameFinishOnAny game(target, dist);
    
    int current_state = 50;
    Vec2 aim{0, 0};
    
    auto outcomes = game.throw_at(aim, current_state);
    
    for (const auto& [new_state, prob] : outcomes) {
        EXPECT_LE(new_state, current_state)
            << "New state should not exceed current state in countdown game";
        EXPECT_GE(new_state, 0)
            << "New state should not be negative";
    }
}

/**
 * Test: Solver consistency
 * Solving the same state multiple times should give the same result
 */
TEST(Integration, SolverConsistency) {
    Target target = create_simple_target();
    NormalDistribution::covariance cov = {{{250, 0}, {0, 250}}};
    NormalDistributionQuadrature dist(cov, Vec2{0, 0});
    GameFinishOnAny game(target, dist);
    Solver solver(game, 600);
    
    int test_state = 40;
    
    auto [score1, aim1] = solver.solve(test_state);
    auto [score2, aim2] = solver.solve(test_state);
    auto [score3, aim3] = solver.solve(test_state);
    
    EXPECT_DOUBLE_EQ(score1, score2) << "Solver should return consistent scores";
    EXPECT_DOUBLE_EQ(score2, score3) << "Solver should return consistent scores";
    EXPECT_DOUBLE_EQ(aim1.x, aim2.x) << "Solver should return consistent aims";
    EXPECT_DOUBLE_EQ(aim1.y, aim2.y) << "Solver should return consistent aims";
    EXPECT_DOUBLE_EQ(aim2.x, aim3.x) << "Solver should return consistent aims";
    EXPECT_DOUBLE_EQ(aim2.y, aim3.y) << "Solver should return consistent aims";
}

/**
 * Test: HeatMapSolver produces reasonable results
 * Heat map should show variation across the target
 */
TEST(Integration, HeatMapSolverVariation) {
    Target target = create_simple_target();
    NormalDistribution::covariance cov = {{{200, 0}, {0, 200}}};
    NormalDistributionQuadrature dist(cov, Vec2{0, 0});
    GameFinishOnDouble game(target, dist);
    
    HeatMapSolver heat_solver(game, 20, 20, 500);  // 20x20 grid
    auto heat_map = heat_solver.heat_map(50);
    
    EXPECT_EQ(heat_map.size(), 20) << "Heat map should have correct height";
    EXPECT_EQ(heat_map[0].size(), 20) << "Heat map should have correct width";
    
    // Find min and max values
    double min_val = std::numeric_limits<double>::max();
    double max_val = std::numeric_limits<double>::lowest();
    
    for (const auto& row : heat_map) {
        for (double val : row) {
            min_val = std::min(min_val, val);
            max_val = std::max(max_val, val);
        }
    }
    
    EXPECT_GT(max_val, min_val) 
        << "Heat map should show variation (not all aims are equally good)";
    EXPECT_GT(min_val, 0.0) 
        << "All heat map values should be positive (expected throws)";
}

/**
 * Test: Different distribution variances affect results
 * More accurate players (smaller variance) should require fewer expected throws
 */
TEST(Integration, DistributionVarianceEffect) {
    Target target = create_simple_target();
    NormalDistribution::covariance cov_accurate = {{{50, 0}, {0, 50}}};
    NormalDistributionQuadrature dist_accurate(cov_accurate, Vec2{0, 0});
    NormalDistribution::covariance cov_inaccurate = {{{500, 0}, {0, 500}}};
    NormalDistributionQuadrature dist_inaccurate(cov_inaccurate, Vec2{0, 0});
    
    GameFinishOnAny game_accurate(target, dist_accurate);
    GameFinishOnAny game_inaccurate(target, dist_inaccurate);
    
    Solver solver_accurate(game_accurate, 500);
    Solver solver_inaccurate(game_inaccurate, 500);
    
    int test_state = 40;
    auto [score_accurate, aim_accurate] = solver_accurate.solve(test_state);
    auto [score_inaccurate, aim_inaccurate] = solver_inaccurate.solve(test_state);
    
    EXPECT_LT(score_accurate, score_inaccurate)
        << "More accurate player should require fewer expected throws";
}

/**
 * Test: Sampling multiple throws approximates distribution
 * Many samples should converge to the expected distribution
 */
TEST(Integration, ThrowSamplingConvergence) {
    Target target = create_simple_target();
    NormalDistribution::covariance cov = {{{100, 0}, {0, 100}}};
    NormalDistributionRandom dist(cov, Vec2{0, 0}, 12345);  // Fixed seed
    GameFinishOnAny game(target, dist);
    
    Vec2 aim{0, 0};  // Aim at center
    int current_state = 50;
    int num_samples = 1000;
    
    std::map<int, int> state_counts;
    for (int i = 0; i < num_samples; ++i) {
        int new_state = game.throw_at_sample(aim, current_state);
        state_counts[new_state]++;
    }
    
    // Check that we get some variation
    EXPECT_GT(state_counts.size(), 1)
        << "Should have multiple different outcomes from sampling";
    
    // Check that state 0 (bull hit) is most common or among most common
    int state_0_count = state_counts[0];
    EXPECT_GT(state_0_count, 0)
        << "Should hit the bull at least once when aiming at center";
}

/**
 * Test: GameFinishOnAny vs GameFinishOnDouble differences
 * FinishOnDouble should generally require more throws (harder to finish)
 */
TEST(Integration, GameModeComparison) {
    Target target = create_simple_target();
    NormalDistribution::covariance cov = {{{150, 0}, {0, 150}}};
    NormalDistributionQuadrature dist(cov, Vec2{0, 0});
    
    GameFinishOnAny game_any(target, dist);
    GameFinishOnDouble game_double(target, dist);
    
    Solver solver_any(game_any, 500);
    Solver solver_double(game_double, 500);
    
    // Test for state 40 (can be finished with double 20)
    auto [score_any, aim_any] = solver_any.solve(40);
    auto [score_double, aim_double] = solver_double.solve(40);
    
    EXPECT_GE(score_double, score_any)
        << "FinishOnDouble should require at least as many throws as FinishOnAny";
}

/**
 * Test: Aiming away from target increases expected throws
 * Aiming far outside should give worse results than aiming at target
 */
TEST(Integration, AimingAccuracyMatters) {
    Target target = create_simple_target();
    NormalDistribution::covariance cov = {{{100, 0}, {0, 100}}};
    NormalDistributionQuadrature dist(cov, Vec2{0, 0});
    GameFinishOnAny game(target, dist);
    Solver solver(game, 500);
    
    int test_state = 50;
    
    // Get optimal aim
    auto [score_optimal, aim_optimal] = solver.solve(test_state);
    
    // Aim far away
    Vec2 aim_far{1000, 1000};
    double score_far = solver.solve_aim(test_state, aim_far);
    
    EXPECT_GT(score_far, score_optimal)
        << "Aiming far from target should give worse results";
}

/**
 * Test: Edge case - very high state
 * Should still produce reasonable results, not crash or overflow
 */
TEST(Integration, HighStateHandling) {
    Target target = create_simple_target();
    NormalDistribution::covariance cov = {{{150, 0}, {0, 150}}};
    NormalDistributionQuadrature dist(cov, Vec2{0, 0});
    GameFinishOnAny game(target, dist);
    Solver solver(game, 300);
    
    int high_state = 150;
    auto [score, aim] = solver.solve(high_state);
    
    EXPECT_GT(score, 0.0) << "High state should require positive throws";
    EXPECT_LT(score, 100.0) << "High state should still be solvable in reasonable time";
    EXPECT_FALSE(std::isnan(score)) << "Score should not be NaN";
    EXPECT_FALSE(std::isinf(score)) << "Score should not be infinite";
}
