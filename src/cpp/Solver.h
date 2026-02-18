#ifndef SOLVER_HEADER
#define SOLVER_HEADER

#include "Geometry.h"
#include "Game.h"

#include <vector>
#include <unordered_map>

/**
 * @defgroup solver Solvers
 * @brief Optimal strategy computation using dynamic programming.
 */

/**
 * @brief Dynamic programming solver for optimal dart throwing strategy.
 * @ingroup solver
 *
 * Computes the minimum expected number of throws to finish from any state,
 * and the optimal aim point for each state. Uses memoization and grid sampling.
 *
 * Algorithm:
 * 1. Sample aim points uniformly over target bounds
 * 2. For each aim, compute expected throws using state transition probabilities
 * 3. Use dynamic programming: solve(s) = min over aims of (1 + expected future throws)
 * 4. Memoize results for efficiency
 *
 * Example usage:
 * @code
 * Solver solver(game, 5000); // 5000 aim samples
 * auto [expected_throws, best_aim] = solver.solve(301);
 * std::cout << "Expected throws: " << expected_throws << std::endl;
 * std::cout << "Aim at: (" << best_aim.x << ", " << best_aim.y << ")" << std::endl;
 * @endcode
 */
class Solver {
public:
    using Score = double; ///< Expected number of throws
    
private:
    static constexpr double EPSILON = 1e-9;
    static constexpr double INFINITE_SCORE = 1e9;  ///< Penalty for unreachable states

    const size_t num_samples_;  ///< Number of aim points to sample
    const Game& game_;
    std::unordered_map<Game::State, std::pair<Score, Vec2>> memoization_;

    /** @brief Generate uniform grid of aim points over target bounds. */
    std::vector<Vec2> sample_aims_() const;
    
public:
    /**
     * @brief Construct solver.
     * @param game Game rules and target
     * @param num_samples Number of aim points to evaluate (default 10000)
     *                    More samples = better solution but slower
     */
    Solver(const Game& game, size_t num_samples = 10000)
        : num_samples_(num_samples), game_(game) {}
    
    /**
     * @brief Compute expected throws for a given state and aim.
     * 
     * Uses formula: E[throws] = (1 + Σ(P(s'|s,aim) * E[throws from s'])) / P(state change)
     * 
     * @param s Current state
     * @param aim Target point
     * @return Expected number of throws to finish
     */
    [[nodiscard]] Score solve_aim(Game::State s, Vec2 aim);
    
    /**
     * @brief Find optimal strategy for a state.
     * 
     * Evaluates all sampled aim points and returns the best.
     * Results are memoized automatically.
     * 
     * @param s State to solve
     * @return (expected_throws, optimal_aim) pair
     */
    [[nodiscard]] std::pair<Score, Vec2> solve(Game::State s);
};

/**
 * @brief Generate heat maps showing expected throws for all aim points.
 * @ingroup solver
 *
 * Visualizes the quality of each possible aim point by computing
 * the expected number of throws if that point is targeted.
 *
 * Example usage:
 * @code
 * HeatMapSolver heatmap(game, 100, 100, 5000); // 100x100 grid, 5000 aim samples
 * auto map = heatmap.heat_map(301);
 * // map[y][x] = expected throws when aiming at grid position (x, y)
 * @endcode
 */
class HeatMapSolver {
public:
    using HeatMap = std::vector<std::vector<Solver::Score>>; ///< Grid of scores [row][col]
private:
    Solver solver_;
    size_t grid_height_;
    size_t grid_width_;
    std::unordered_map<Game::State, HeatMap> heat_map_memo_;
    std::pair<Vec2, Vec2> target_bounds_;

public:
    /**
     * @brief Construct heat map generator.
     * @param game Game rules and target
     * @param grid_height Number of rows in heat map
     * @param grid_width Number of columns in heat map
     * @param num_samples Number of aim samples for solver (default 10000)
     */
    HeatMapSolver(const Game& game, size_t grid_height, size_t grid_width, size_t num_samples = 10000)
        : solver_(game, num_samples), grid_height_(grid_height), grid_width_(grid_width) {
            target_bounds_ = game.get_target_bounds();
        }

    /**
     * @brief Generate heat map for a game state.
     * 
     * Each cell contains the expected number of throws when aiming at
     * the center of that cell. Lower values indicate better aim points.
     * 
     * @param s Game state
     * @return Grid of expected throws [row][col]
     */
    [[nodiscard]] HeatMap heat_map(Game::State s);
};

#endif