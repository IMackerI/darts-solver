#ifndef SOLVER_HEADER
#define SOLVER_HEADER

#include "Geometry.h"
#include "Game.h"

#include <unordered_set>
#include <vector>
#include <unordered_map>
#include <cstdint>

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
 * @brief Dynamic programming solver for round-based optimal dart throwing strategy.
 * @ingroup solver
 *
 * Computes the minimum expected number of rounds to finish from any state.
 * Within a round, the player gets a fixed number of throws. If a throw causes an 
 * invalid/busted state (such as dropping below 0, or below 2 in Double Out), the 
 * remaining throws are lost and the score is reset to the state at the start of the round.
 */
class SolverMinRounds : public Solver {
private:
    static constexpr double EPSILON = 1e-5;
    static constexpr double INFINITE_SCORE = 1e9;

    struct RoundStateKey {
        Game::State round_start_score;
        Game::State current_score;
        unsigned int throw_number;

        bool operator==(const RoundStateKey& other) const {
            return round_start_score == other.round_start_score
                && current_score == other.current_score
                && throw_number == other.throw_number;
        }
    };

    struct RoundStateKeyHash {
        size_t operator()(const RoundStateKey& key) const {
            size_t h1 = std::hash<Game::State>{}(key.round_start_score);
            size_t h2 = std::hash<Game::State>{}(key.current_score);
            size_t h3 = std::hash<unsigned int>{}(key.throw_number);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };

    unsigned int throws_per_round_;
    std::unordered_map<RoundStateKey, std::pair<Score, Vec2>, RoundStateKeyHash> memoization_;
    std::unordered_set<Game::State> winable_;
    std::unordered_map<uint64_t, double> round_dp_cache_;

    // Evaluates the expected rounds from state `current_score` with `throws_left` darts remaining.
    // `X_start_guess` is the assumed total expected rounds from `start_score` (used when busting).
    // Uses its own internal memoization cache for the current mini-DP step.
    double evaluate_dp(Game::State start_score, Game::State current_score, unsigned int throws_left, double X_start_guess,
                       std::unordered_map<unsigned int, double>& inner_memo);

    std::pair<Score, Vec2> solve_round_start_state(Game::State s);
    std::pair<Score, Vec2> solve_nonstart_round_state(Game::State round_start_score, Game::State current_score, unsigned int throw_number);
    [[nodiscard]] bool is_valid_throw_number(unsigned int throw_number) const;
    [[nodiscard]] RoundStateKey make_state_key(Game::State round_start_score, Game::State current_score, unsigned int throw_number) const;
    [[nodiscard]] uint64_t make_round_dp_cache_key(Game::State round_start_score, Game::State current_score, unsigned int throws_left) const;
    double evaluate_dp_cached(Game::State start_score, Game::State current_score, unsigned int throws_left, double round_start_value);

public:
    /**
     * @brief Construct solver.
     * @param game Game rules and target
     * @param throws_per_round The number of throws allowed in a single round
     * @param num_samples Number of aim points to evaluate
     */
    SolverMinRounds(const Game& game, unsigned int throws_per_round = 3, size_t num_samples = 10000)
        : Solver(game, num_samples), throws_per_round_(throws_per_round) {}

    /**
     * @brief Compute expected rounds for a given state and aim for the FIRST dart of the round.
     * 
     * @param s Current starting state of the round
     * @param aim Target point
     * @return Expected number of rounds to finish
     */
    [[nodiscard]] Score solve_aim(Game::State s, Vec2 aim) override;

    /**
     * @brief Compute expected rounds for a given in-round state and aim.
     *
     * This supports solving from the middle of a round. The current round is always
     * counted as one round in the returned expectation, matching the start-of-round API.
     *
     * @param round_start_score Score at the start of the current round
     * @param current_score Current remaining score before this throw
     * @param throw_number Current throw number in the round (1..throws_per_round)
     * @param aim Target point
     * @return Expected number of rounds to finish
     */
    [[nodiscard]] Score solve_aim_round_state(Game::State round_start_score, Game::State current_score,
                                              unsigned int throw_number, Vec2 aim);
    
    /**
     * @brief Find optimal strategy for a state.
     * 
     * @param s State to solve
     * @return (expected_rounds, optimal_aim_for_first_dart) pair
     */
    std::pair<Score, Vec2> solve(Game::State s) override;

    /**
     * @brief Find optimal strategy for an arbitrary in-round state.
     *
     * @param round_start_score Score at the start of the current round
     * @param current_score Current remaining score before this throw
     * @param throw_number Current throw number in the round (1..throws_per_round)
     * @return (expected_rounds, optimal_aim_for_selected_throw)
     */
    [[nodiscard]] std::pair<Score, Vec2> solve_round_state(Game::State round_start_score, Game::State current_score,
                                                           unsigned int throw_number);
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