#include "Solver.h"
#include "Random.h"

#include <utility>
#include <vector>
#include <random>

std::vector<Vec2> Solver::sample_aims() const {
    std::vector<Vec2> aims;
    auto [min_point, max_point] = game.get_target_bounds();

    std::uniform_real_distribution<double> x_dist(min_point.x, max_point.x);
    std::uniform_real_distribution<double> y_dist(min_point.y, max_point.y);

    for (size_t i = 0; i < NumSamples; ++i) {
        aims.emplace_back(x_dist(random_engine), y_dist(random_engine));
    }

    return aims;
}

Solver::Score Solver::expected_score(Game::State s, Vec2 aim) {
    auto states = game.throw_at(aim, s);
    Solver::Score expected = 0;
    double same_state_prob = 0;

    for (const auto& [state, probability] : states) {
        if (state == s) {
            same_state_prob += probability;
            continue;
        }
        
        expected += solve(state).first * probability;
    }

    if (same_state_prob >= 1.0 - EPSILON) return INFINITE_SCORE;
    
    expected = (expected + 1) / (1.0 - same_state_prob);
    return expected;
}

std::pair<Solver::Score, Vec2> Solver::solve(Game::State s) {
    if (s == 0) return {0, Vec2{0,0}};
    if (memoization.contains(s)) { return memoization[s]; }

    std::pair<Solver::Score, Vec2> best_score = {INFINITE_SCORE, Vec2{0,0}};

    for (const auto& aim : sample_aims()) {
        Solver::Score score = expected_score(s, aim);
        if (score < best_score.first) {
            best_score = {score, aim};
        }
    }
    return memoization[s] = best_score;
}

