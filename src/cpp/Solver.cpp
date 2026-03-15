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

double SolverMinRounds::evaluate_dp(Game::State start_score, Game::State current_score, unsigned int throws_left, double X_start_guess, std::unordered_map<unsigned int, double>& inner_memo) {
    if (current_score == 0) return 0.0;
    if (throws_left == 0) {
        if (current_score == start_score) return X_start_guess;
        double cost = solve(current_score).first;
        if (!winable_.contains(current_score)) return X_start_guess; // Treat unwinnable as bust
        return cost;
    }

    // State packing just for internal memoization inside this single round
    unsigned int state_key = (current_score << 8) | throws_left;
    if (inner_memo.contains(state_key)) {
        return inner_memo[state_key];
    }

    double best_expected = INFINITE_SCORE;

    for (const auto& aim : sample_aims_()) {
        auto states = game_.throw_at(aim, current_score);
        auto hits = game_.throw_at_distribution(aim);

        double expected = 0.0;
        for (size_t i = 0; i < states.size(); ++i) {
            Game::State next_state = states[i].first;
            double prob = states[i].second;
            HitData hit = hits[i].first;

            if (next_state == 0) {
                expected += prob * 0.0;
            } else if (hit.diff == 0) { // Miss, not a bust
                expected += prob * evaluate_dp(start_score, current_score, throws_left - 1, X_start_guess, inner_memo);
            } else {
                // Not a miss, check if bust
                bool is_bust = (next_state == current_score) || (next_state != 0 && !winable_.contains(next_state));
                // Add case: if current_score is NOT start_score, and we hit something that leaves score unchanged but diff != 0?
                // Actually `next_state == current_score` for hit.diff != 0 is ALWAYS an overshoot (bust).
                if (is_bust) {
                    expected += prob * X_start_guess;
                } else {
                    expected += prob * evaluate_dp(start_score, next_state, throws_left - 1, X_start_guess, inner_memo);
                }
            }
        }
        if (expected < best_expected) {
            best_expected = expected;
        }
    }

    inner_memo[state_key] = best_expected;
    return best_expected;
}

SolverMinRounds::Score SolverMinRounds::solve_aim(Game::State s, Vec2 aim) {
    // This is queried mostly for outputting visualization or manually choosing an aim.
    // We assume it's the start of the round, so throws_left = throws_per_round_.
    // However, if we aren't solved yet for s, we return INFINITE_SCORE since we don't know the future.
    if (!memoization_.contains(s)) return INFINITE_SCORE;
    double X_start_guess = memoization_[s].first;

    std::unordered_map<unsigned int, double> inner_memo;
    
    auto states = game_.throw_at(aim, s);
    auto hits = game_.throw_at_distribution(aim);

    double expected = 0.0;
    for (size_t i = 0; i < states.size(); ++i) {
        Game::State next_state = states[i].first;
        double prob = states[i].second;
        HitData hit = hits[i].first;

        if (next_state == 0) {
            expected += prob * 0.0;
        } else if (hit.diff == 0) {
            expected += prob * evaluate_dp(s, s, throws_per_round_ - 1, X_start_guess, inner_memo);
        } else {
            bool is_bust = (next_state == s) || (next_state != 0 && !winable_.contains(next_state));
            if (is_bust) {
                expected += prob * X_start_guess;
            } else {
                expected += prob * evaluate_dp(s, next_state, throws_per_round_ - 1, X_start_guess, inner_memo);
            }
        }
    }
    // Expected future rounds if we take this aim + 1 for the current round
    return expected + 1.0; 
}

std::pair<SolverMinRounds::Score, Vec2> SolverMinRounds::solve(Game::State s) {
    if (s == 0) {
        winable_.insert(0);
        return {0.0, Vec2{0.0, 0.0}};
    }

    if (memoization_.contains(s)) {
        return memoization_[s];
    }

    // Pre-evaluate all strictly smaller states to populate winable_ and memoization_
    for (const auto& aim : sample_aims_()) {
        auto states = game_.throw_at(aim, s);
        for (const auto& state_prob : states) {
            Game::State next_state = state_prob.first;
            if (next_state < s && next_state != 0) {
                solve(next_state);
            }
        }
    }

    // Fixed-point iteration to resolve the score self-loop
    double current_guess = s / 20.0 + 1.0; // Reasonable upper bound
    double last_guess = 0;
    std::pair<SolverMinRounds::Score, Vec2> best_score = {INFINITE_SCORE, Vec2{0.0, 0.0}};
    int iterations = 0;

    while (std::abs(current_guess - last_guess) > EPSILON && iterations < 50) {
        last_guess = current_guess;
        double best_expected = INFINITE_SCORE;
        Vec2 best_aim{0.0, 0.0};
        
        std::unordered_map<unsigned int, double> inner_memo;

        for (const auto& aim : sample_aims_()) {
            auto states = game_.throw_at(aim, s);
            auto hits = game_.throw_at_distribution(aim);

            double expected = 0.0;
            for (size_t i = 0; i < states.size(); ++i) {
                Game::State next_state = states[i].first;
                double prob = states[i].second;
                HitData hit = hits[i].first;

                if (next_state == 0) {
                    expected += prob * 0.0;
                } else if (hit.diff == 0) { // Miss
                    expected += prob * evaluate_dp(s, s, throws_per_round_ - 1, current_guess, inner_memo);
                } else {
                    bool is_bust = (next_state == s) || (next_state != 0 && !winable_.contains(next_state));
                    if (is_bust) {
                        expected += prob * current_guess;
                    } else {
                        expected += prob * evaluate_dp(s, next_state, throws_per_round_ - 1, current_guess, inner_memo);
                    }
                }
            }
            
            if (expected < best_expected) {
                best_expected = expected;
                best_aim = aim;
            }
        }
        
        current_guess = best_expected + 1.0;
        best_score = {current_guess, best_aim};
        iterations++;
    }

    if (best_score.first < INFINITE_SCORE - 1000.0) {
        winable_.insert(s);
    }
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
