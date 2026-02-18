
@mainpage Darts Solver Documentation

@section intro Introduction
This library finds the optimal aiming strategy for darts games using
dynamic programming and probability theory. Given a dartboard layout and
a probability distribution modeling throwing accuracy, the solver computes
the best point to aim at for any game state to minimize the expected 
number of throws to finish.

@section modules Modules
The library is organized into the following modules:

- @ref geometry - 2D vectors and polygons for representing dartboards
- @ref distributions - Probability distributions for modeling throwing accuracy
- @ref game - Game rules and dartboard targets
- @ref solver - Optimal strategy computation

@section quickstart Quick Start
@code
// Load a dartboard
Target board("dartboard.txt");

// Create throwing distribution (2D Gaussian with std dev = 2)
NormalDistribution::covariance cov = {{{2.0, 0.0}, {0.0, 2.0}}};
NormalDistributionRandom dist(cov, Vec2{0,0}, 10000);

// Set up game with standard darts rules
GameFinishOnDouble game(board, dist);

// Solve for optimal strategy from 501 points
Solver solver(game, 5000);
auto [expected_throws, best_aim] = solver.solve(501);

std::cout << "Expected throws to finish: " << expected_throws << std::endl;
std::cout << "Best aim point: (" << best_aim.x << ", " << best_aim.y << ")" << std::endl;
@endcode

@section features Key Features

- **Multiple integration methods**: Choose between Monte Carlo (fast, approximate)
  and Gauss quadrature (exact for polynomial densities)
- **Flexible game rules**: Support for standard finish-on-double rules or simpler variants
- **Efficient solving**: Dynamic programming with memoization for optimal performance
- **Heat map generation**: Visualize the quality of all possible aim points

@section algorithm Algorithm Overview

The solver uses a dynamic programming approach:
1. Sample a grid of candidate aim points across the dartboard
2. For each aim point and state, compute state transition probabilities
3. Use Bellman equation: E[throws(s)] = min over aims of (1 + Σ P(s'|s,aim) * E[throws(s')])
4. Memoize results to avoid recomputation

The probability calculations account for throwing accuracy by integrating
the probability distribution over each scoring region of the dartboard.

