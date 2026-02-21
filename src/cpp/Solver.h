#ifndef SOLVER_HEADER
#define SOLVER_HEADER

#include "Geometry.h"
#include "Game.h"

#include <unordered_set>
#include <vector>
#include <unordered_map>

/**
 * @defgroup solver Solvers
 * @brief Optimal strategy computation for dart throwing.
 * 
 * Provides multiple solver strategies:
 * - SolverMinThrows: Minimize expected throws to win (dynamic programming)
 * - MaxPointsSolver: Maximize expected points per throw (greedy)
 * - HeatMapVisualizer: Visualize solver output across the target
 */

/**
 * @brief Abstract base class for dart throwing solvers.
 * @ingroup solver
 * 
 * Provides common functionality for sampling aim points and evaluating strategies.
 */
class Solver {
public:
    using Score = double; ///< Expected number of throws

protected:
    /** @brief Generate uniform grid of aim points over target bounds. */
    std::vector<Vec2> sample_aims_() const;

    const size_t num_samples_;  ///< Number of aim points to sample
    const Game& game_;

public:
    /**
     * @brief Construct solver.
     * @param game Game rules and target
     * @param num_samples Number of aim points to evaluate (default 10000)
     *                    More samples = better solution but slower
     */
    Solver(const Game& game, size_t num_samples = 10000)
        : num_samples_(num_samples), game_(game) {}

    virtual ~Solver() = default;
    [[nodiscard]] virtual std::pair<double, Vec2> solve(Game::State s) = 0;
    [[nodiscard]] virtual double solve_aim(Game::State s, Vec2 aim) = 0;
    [[nodiscard]] const Game& get_game() const { return game_; }
};
 
/**
 * @brief Dynamic programming solver for optimal dart throwing strategy.
 * @ingroup solver
 *
 * Computes the minimum expected number of throws to finish from any state,
 * and the optimal aim point for each state. Uses memoization and grid sampling.
 * Takes into account future game states when evaluating aim points.
 *
 * Algorithm:
 * 1. Sample aim points uniformly over target bounds
 * 2. For each aim, compute expected throws using state transition probabilities
 * 3. Use dynamic programming: solve(s) = min over aims of (1 + expected future throws)
 * 4. Memoize results for efficiency
 *
 * Example usage:
 * @code
 * SolverMinThrows solver(game, 5000); // 5000 aim samples
 * auto [expected_throws, best_aim] = solver.solve(301);
 * std::cout << "Expected throws: " << expected_throws << std::endl;
 * std::cout << "Aim at: (" << best_aim.x << ", " << best_aim.y << ")" << std::endl;
 * @endcode
 */
class SolverMinThrows : public Solver {
private:
    static constexpr double EPSILON = 1e-9;
    static constexpr double INFINITE_SCORE = 1e9;  ///< Penalty for unreachable states

    std::unordered_map<Game::State, std::pair<Score, Vec2>> memoization_;
    std::unordered_set<Game::State> winable_;

public:
    using Solver::Solver;
    
    /**
     * @brief Compute expected throws for a given state and aim.
     * 
     * Uses formula: E[throws] = (1 + Σ(P(s'|s,aim) * E[throws from s'])) / P(state change)
     * 
     * @param s Current state
     * @param aim Target point
     * @return Expected number of throws to finish
     */
    [[nodiscard]] Score solve_aim(Game::State s, Vec2 aim) override;
    
    /**
     * @brief Find optimal strategy for a state.
     * 
     * Evaluates all sampled aim points and returns the best.
     * Results are memoized automatically.
     * 
     * @param s State to solve
     * @return (expected_throws, optimal_aim) pair
     */
    [[nodiscard]] std::pair<Score, Vec2> solve(Game::State s) override;
};
/**
 * @brief Greedy solver that maximizes expected points per throw.
 * @ingroup solver
 *
 * Finds the aim point that maximizes the expected score reduction per throw.
 * Unlike SolverMinThrows, this doesn't consider future game states - it's purely
 * greedy and aims for the highest expected value each throw.
 *
 * This is useful for:
 * - Analyzing optimal aiming strategies without considering game state
 * - Comparing greedy vs. optimal (DP) strategies
 * - Situations where maximizing current score is the goal
 *
 * Example usage:
 * @code
 * MaxPointsSolver solver(game, 1000); // 1000 aim samples
 * auto [expected_points, best_aim] = solver.solve(301);
 * std::cout << "Expected points: " << expected_points << std::endl;
 * std::cout << "Aim at: (" << best_aim.x << ", " << best_aim.y << ")" << std::endl;
 * @endcode
 */
class MaxPointsSolver : public Solver {
private:
    static constexpr double LOWEST_SCORE = 0.0;  ///< Minimum possible score (missing the board)
public:
    using Score = double; ///< Expected points scored

    using Solver::Solver;
    
    /**
     * @brief Compute expected points for a given state and aim.
     * @param s Current state (unused - greedy doesn't consider state)
     * @param aim Target point
     * @return Expected points scored
     */
    [[nodiscard]] Score solve_aim(Game::State s, Vec2 aim) override;
    
    /**
     * @brief Find aim point that maximizes expected score.
     * @param s Current state (unused - greedy doesn't consider state)
     * @return (expected_points, optimal_aim) pair
     */
    [[nodiscard]] std::pair<Score, Vec2> solve(Game::State s) override;
};

/**
 * @brief Generate heat maps showing expected throws for all aim points.
 * @ingroup solver
 *
 * Visualizes the quality of each possible aim point by computing
 * the Score the given solver would achieve when aiming at that point.
 *
 * Example usage:
 * @code
 * MaxPointsSolver solver(game, 1000);
 * HeatMapVisualizer heatmap(solver, 100, 100); // 100x100 grid
 * auto map = heatmap.heat_map(1000000); // Shows how much you can score in general (in this high state, you won't overshoot)
 * // map[row][col] = expected number of points scored when aiming at that cell's center
 * @endcode
 * You can also use SolverMinThrows to see expected throws instead of points
 */
class HeatMapVisualizer {
public:
    using HeatMap = std::vector<std::vector<Solver::Score>>; ///< Grid of scores [row][col]
    using Bounds = Game::Bounds; ///< Bounding box of target (min, max)
private:
    Solver& solver_;
    size_t grid_height_;
    size_t grid_width_;
    std::unordered_map<Game::State, HeatMap> heat_map_memo_;
    Bounds target_bounds_;

public:
    /**
     * @brief Construct heat map generator.
     * @param solver Solver to use for evaluating aim points
     * @param grid_height Number of rows in heat map
     * @param grid_width Number of columns in heat map
     */
    HeatMapVisualizer(Solver& solver, size_t grid_height, size_t grid_width)
        : solver_(solver), grid_height_(grid_height), grid_width_(grid_width) {
            target_bounds_ = solver.get_game().get_target_bounds();
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