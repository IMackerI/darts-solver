#ifndef SOLVER_HEADER
#define SOLVER_HEADER

#include "Distribution.h"
#include "Geometry.h"
#include "Target.h"

#include <vector>
#include <unordered_map>

class Solver {
    std::unordered_map<state, score> memoization;
    const Distribution& distribution;
public:
    Solver(const Distribution& distribution);
    score expected_score(state s, Point aim);
    score solve(state s);
};

#endif