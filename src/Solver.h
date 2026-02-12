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
    std::unordered_map<Game::State, Score> memoization;
    const Distribution& distribution;
public:
    Solver(const Distribution& distribution);
    Score expected_score(Game::State s, Point aim);
    Score solve(Game::State s);
};

#endif