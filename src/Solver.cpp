#include "Solver.h"

#include <utility>
#include <vector>
#include <cmath>

std::vector<Vec2> Solver::sample_aims_() const {
    std::vector<Vec2> aims;
    auto [min_point, max_point] = game_.get_target_bounds();

    size_t height_samples = static_cast<size_t>(std::sqrt(num_samples_));
    size_t width_samples = num_samples_ / height_samples;

    for (size_t i = 0; i < width_samples; ++i) {
        for (size_t j = 0; j < height_samples; ++j) {
            double x = min_point.x + (max_point.x - min_point.x) * (i + 0.5) / width_samples;
            double y = min_point.y + (max_point.y - min_point.y) * (j + 0.5) / height_samples;
            aims.emplace_back(x, y);
        }
    }

    return aims;
}

Solver::Score Solver::solve_aim(Game::State s, Vec2 aim) {
    auto states = game_.throw_at(aim, s);
    Solver::Score expected = 0;
    double same_state_prob = 0;

    for (const auto& [state, probability] : states) {
        if (state == s) {
            same_state_prob += probability;
            continue;
        }
        
        expected += solve(state).first * probability;
    }

    if (same_state_prob >= 1.0 - EPSILON) {
        return INFINITE_SCORE;
    }
    
    // Account for the current throw and normalize by probability of state change
    expected = (expected + 1.0) / (1.0 - same_state_prob);
    return expected;
}

std::pair<Solver::Score, Vec2> Solver::solve(Game::State s) {
    if (s == 0) {
        return {0.0, Vec2{0.0, 0.0}};
    }
    
    if (memoization_.contains(s)) {
        return memoization_[s];
    }

    std::pair<Solver::Score, Vec2> best_score = {INFINITE_SCORE, Vec2{0.0, 0.0}};

    for (const auto& aim : sample_aims_()) {
        Solver::Score score = solve_aim(s, aim);
        if (score < best_score.first) {
            best_score = {score, aim};
        }
    }
    
    memoization_[s] = best_score;
    return best_score;
}

[[nodiscard]]  HeatMapSolver::HeatMap HeatMapSolver::heat_map(Game::State s) {
    if (heat_map_memo_.contains(s)) {
        return heat_map_memo_[s];
    }
    HeatMap heat_map(grid_height_, std::vector<Solver::Score>(grid_width_, 0.0));

    auto [min_point, max_point] = target_bounds_;

    for (size_t i = 0; i < grid_width_; ++i) {
        for (size_t j = 0; j < grid_height_; ++j) {
            double x = min_point.x + (max_point.x - min_point.x) * (i + 0.5) / grid_width_;
            double y = min_point.y + (max_point.y - min_point.y) * (j + 0.5) / grid_height_;
            heat_map[j][i] = solver_.solve_aim(s, Vec2{x, y});
        }
    }

    heat_map_memo_[s] = heat_map;
    return heat_map;
}
