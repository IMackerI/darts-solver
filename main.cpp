#include "Game.h"
#include "Solver.h"
#include "src/Distribution.h"
#include "src/Geometry.h"
#include "src/Random.h"

#include <iostream>

void try_avg_dist(NormalDistributionRandom dist) {
    
    double avg_dist = 0;
    for (int i = 0; i < 1000; ++i) {
        Point p = dist.sample();
        avg_dist += std::sqrt(p.x * p.x + p.y * p.y); 
    }
    avg_dist /= 1000;
    std::cout << "Average distance from mean: " << avg_dist << std::endl;
}

int main() {
    NormalDistributionRandom::covariance cov = {{{400, 0}, {0, 400}}};
    NormalDistributionRandom dist(cov, Point{0, 0}, 1000);
    try_avg_dist(dist);

    Target target("target.txt");
    Game game(target, dist);
    Solver solver(game, 100);
    
    for (int state = 0; state <= 101; ++state) {
        Solver::Score score = solver.solve(state);
        std::cout << "State: " << state << ", Expected throws to finish: " << score << std::endl;
    }

}