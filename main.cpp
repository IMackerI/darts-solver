#include "Game.h"
#include "Solver.h"
#include "Distribution.h"
#include "Geometry.h"

#include <cmath>
#include <iostream>


void try_avg_dist(NormalDistribution* dist, int NUM_SAMPLE_ITERATIONS = 10000) {
    double avg_dist = 0;
    for (int i = 0; i < NUM_SAMPLE_ITERATIONS; ++i) {
        Vec2 p = dist->sample();
        avg_dist += std::sqrt(p.x * p.x + p.y * p.y);
    }
    avg_dist /= NUM_SAMPLE_ITERATIONS;
    std::cout << "Average distance from mean: " << avg_dist << std::endl;
}

void print_results(Solver &solver, Game &game, int MAX_DARTS_STATE = 101, size_t GRID_HEIGHT = 100, size_t GRID_WIDTH = 100) {

    HeatMapSolver heat_map_solver(game, GRID_HEIGHT, GRID_WIDTH);
    auto [min_point, max_point] = game.get_target_bounds();
    
    for (int state = 1; state <= MAX_DARTS_STATE; ++state) {
        auto [score, aim] = solver.solve(state);
        std::cout << "State: " << state << std::endl;
        std::cout << "Expected throws to finish: " << score 
            << ", Best aim: (" << aim.x << ", " << aim.y << ")" << std::endl;

        auto heat_map = heat_map_solver.heat_map(state);
        std::cout << "Heat map for state " << state << ":\n";
        std::cout << "Heat map extent: " << min_point.x << " " << min_point.y << " " 
                  << max_point.x << " " << max_point.y << "\n";
        for (const auto& row : heat_map) {
            for (const auto& cell : row) {
                std::cout << cell << " ";
            }
            std::cout << "\n";
        }
        std::cout << std::endl;
        std::cerr << "Finished state " << state << std::endl;
    }

}

int main() {
    NormalDistribution::covariance cov = {{{1600, 0}, {0, 1600}}};
    NormalDistributionQuadrature dist(cov, Vec2{0, 0});
    try_avg_dist(&dist);
    Target target("target.out");
    GameFinishOnDouble game(target, dist);
    Solver solver(game, 10000);

    print_results(solver, game);
    return 0;
}