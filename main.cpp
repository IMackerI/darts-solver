#include "Game.h"
#include "Solver.h"
#include "Distribution.h"
#include "Geometry.h"

#include <cmath>
#include <iostream>

namespace {
    constexpr int NUM_SAMPLE_ITERATIONS = 1000;
    constexpr int MAX_DARTS_STATE = 101;
}

void try_avg_dist(NormalDistribution* dist) {
    double avg_dist = 0;
    for (int i = 0; i < NUM_SAMPLE_ITERATIONS; ++i) {
        Vec2 p = dist->sample();
        avg_dist += std::sqrt(p.x * p.x + p.y * p.y);
    }
    avg_dist /= NUM_SAMPLE_ITERATIONS;
    std::cout << "Average distance from mean: " << avg_dist << std::endl;
}

void print_results(Solver &solver) {
    for (int state = 0; state <= MAX_DARTS_STATE; ++state) {
        auto [score, aim] = solver.solve(state);
        std::cout << "State: " << state << ", Expected throws to finish: " 
            << score << ", Best aim: (" << aim.x << ", " << aim.y << ")" << std::endl;
    }
}

int main() {
    NormalDistribution::covariance cov = {{{200, 0}, {0, 200}}};
    NormalDistributionQuadrature dist(cov, Vec2{0, 0});
    try_avg_dist(&dist);
    Target target("target.out");
    GameFinishOnAny game(target, dist);
    Solver solver(game, 10000);

    print_results(solver);
    return 0;
}