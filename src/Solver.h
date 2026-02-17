#ifndef SOLVER_HEADER
#define SOLVER_HEADER

#include "Geometry.h"
#include "Game.h"

#include <vector>
#include <unordered_map>

class Solver {
public:
    using Score = double;
    
private:
    static constexpr double EPSILON = 1e-9;
    static constexpr double INFINITE_SCORE = 1e9;

    const size_t num_samples_;
    const Game& game_;
    std::unordered_map<Game::State, std::pair<Score, Vec2>> memoization_;

    std::vector<Vec2> sample_aims_() const;
    
public:
    Solver(const Game& game, size_t num_samples = 10000)
        : num_samples_(num_samples), game_(game) {}
    
    [[nodiscard]] Score solve_aim(Game::State s, Vec2 aim);
    [[nodiscard]] std::pair<Score, Vec2> solve(Game::State s);
};

class HeatMapSolver {
public:
    using HeatMap = std::vector<std::vector<Solver::Score>>;
private:
    Solver solver_;
    size_t grid_height_;
    size_t grid_width_;
    std::unordered_map<Game::State, HeatMap> heat_map_memo_;
    std::pair<Vec2, Vec2> target_bounds_;

public:
    HeatMapSolver(const Game& game, size_t grid_height, size_t grid_width, size_t num_samples = 10000)
        : solver_(game, num_samples), grid_height_(grid_height), grid_width_(grid_width) {
            target_bounds_ = game.get_target_bounds();
        }

    [[nodiscard]] HeatMap heat_map(Game::State s);
};

#endif