#ifndef SOLVER_HEADER
#define SOLVER_HEADER

#include "Distribution.h"
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

    const size_t NumSamples;

    std::unordered_map<Game::State, Score> memoization;
    const Game& game;

    std::vector<Point> sample_aims() const;
public:
    Solver(const Game& game, size_t num_samples = 10000) : NumSamples(num_samples), game(game) {};
    Score expected_score(Game::State s, Point aim);
    Score solve(Game::State s);
};

#endif