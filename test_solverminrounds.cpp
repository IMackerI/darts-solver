#include "src/cpp/Solver.h"
#include "src/cpp/Game.h"
#include "src/cpp/Geometry.h"
#include "src/cpp/Distribution.h"
#include <iostream>
#include <sstream>

int main() {
    std::stringstream input("2\n"
                            "20 normal 4 0 0 10 0 10 10 0 10\n"
                            "40 double 4 10 0 20 0 20 10 10 10\n");
    Target board(input);
    NormalDistribution::covariance cov = {{{10.0, 0.0}, {0.0, 10.0}}};
    NormalDistributionRandom dist(cov, Vec2{0,0}, 500);
    GameFinishOnDouble game(board, dist);
    
    SolverMinRounds solver(game, 3, 400); // fewer samples for speed
    
    // Evaluate starting from 2
    for(int s=2; s<=40; ++s) { solver.solve(s); }
    
    std::cout << "S=20: " << solver.solve(20).first << " rounds" << std::endl;
    std::cout << "S=40: " << solver.solve(40).first << " rounds" << std::endl;
    
    return 0;
}
