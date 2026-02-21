#include "Solver.h"
#include "Game.h"

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

SolverMinThrows::Score SolverMinThrows::solve_aim(Game::State s, Vec2 aim) {
    auto states = game_.throw_at(aim, s);
    SolverMinThrows::Score expected = 0;
    double same_state_prob = 0;

    for (const auto& [state, probability] : states) {
        if (state == s) {
            same_state_prob += probability;
            continue;
        }
        SolverMinThrows::Score state_score = solve(state).first;

        if (!winable_.contains(state)) {
            same_state_prob += probability;
            continue;
        }

        expected += state_score * probability;
    }

    if (same_state_prob >= 1.0 - EPSILON) {
        return INFINITE_SCORE;
    }
    
    // Account for the current throw and normalize by probability of state change
    expected = (expected + 1.0) / (1.0 - same_state_prob);
    return expected;
}

std::pair<SolverMinThrows::Score, Vec2> SolverMinThrows::solve(Game::State s) {
    if (s == 0) {
        winable_.insert(0);
        return {0.0, Vec2{0.0, 0.0}};
    }
    
    if (memoization_.contains(s)) {
        return memoization_[s];
    }

    std::pair<SolverMinThrows::Score, Vec2> best_score = {INFINITE_SCORE, Vec2{0.0, 0.0}};
    bool is_winable = false;

    for (const auto& aim : sample_aims_()) {
        SolverMinThrows::Score score = solve_aim(s, aim);
        if (score < best_score.first) {
            best_score = {score, aim};
        }
        if (score < INFINITE_SCORE) {
            is_winable = true;
        }
    }
    
    if (is_winable) winable_.insert(s);
    memoization_[s] = best_score;

    return best_score;
}

MaxPointsSolver::Score MaxPointsSolver::solve_aim(Game::State s, Vec2 aim) {
    auto states = game_.throw_at(aim, s);
    MaxPointsSolver::Score expected = 0;

    for (const auto& [state, probability] : states) {
        Game::StateDifference diff = s - state;
        expected += diff * probability;
    }

    return expected;
}

std::pair<MaxPointsSolver::Score, Vec2> MaxPointsSolver::solve(Game::State s) {
    std::pair<MaxPointsSolver::Score, Vec2> best_score = {MaxPointsSolver::LOWEST_SCORE, Vec2{0.0, 0.0}};

    for (const auto& aim : Solver::sample_aims_()) {
        MaxPointsSolver::Score score = solve_aim(s, aim);
        if (score > best_score.first) {
            best_score = {score, aim};
        }
    }

    return best_score;
}

[[nodiscard]] HeatMapVisualizer::HeatMap HeatMapVisualizer::heat_map(Game::State s) {
    if (heat_map_memo_.contains(s)) {
        return heat_map_memo_[s];
    }
    HeatMap heat_map(grid_height_, std::vector<typename SolverMinThrows::Score>(grid_width_, 0.0));

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
