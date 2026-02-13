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

#endif