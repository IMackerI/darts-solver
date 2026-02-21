#ifndef GAME_HEADER
#define GAME_HEADER

#include "Distribution.h"
#include "Geometry.h"
#include <unordered_map>
#include <utility>
#include <vector>
#include <string>
#include <limits>

/**
 * @defgroup game Game Rules and Targets
 * @brief Game logic, scoring rules, and dartboard target definitions.
 */

class Target;

struct HitData;

/**
 * @brief Abstract base class for darts game logic.
 *
 * Combines a target (dart board layout) with a throwing distribution
 * to simulate throws and compute state transitions.
 *
 * Example usage:
 * @code
 * Target standard_board("dartboard.txt");
 * NormalDistributionRandom dist(cov, Vec2{0,0}, 5000);
 * GameFinishOnDouble game(standard_board, dist);
 * 
 * auto outcomes = game.throw_at(Vec2{10, 5}, 501); // Throw at (10,5) from 501
 * // Returns vector of (new_state, probability) pairs
 * @endcode
 */
class Game {
public:
    using State = unsigned int; ///< Game state (typically points remaining)
    using StateDifference = int; ///< Change in score (negative reduces remaining points)
    using HitDistribution = std::vector<std::pair<HitData, double>>; ///< Hit outcomes with probabilities
    struct Bounds {
        Vec2 min;
        Vec2 max;
    };
protected:
    const Target& target_;
    const Distribution& distribution_;
    
    mutable std::unordered_map<Vec2, HitDistribution> throw_at_cache_;
    mutable Bounds target_bounds_ = {
        Vec2{std::numeric_limits<double>::max(), std::numeric_limits<double>::max()},
        Vec2{std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest()}
    };

    /** @brief Compute probability distribution of hits when aiming at p. Cached. */
    HitDistribution throw_at_distribution_(Vec2 p) const;
public:
    Game(const Target& target, const Distribution& distribution);
    
    /**
     * @brief Get bounding box of target with 10% padding.
     * @return (min_corner, max_corner) pair
     */
    [[nodiscard]] Bounds get_target_bounds() const;
    
    virtual ~Game() = default;
    
    /**
     * @brief Simulate a single sampled throw.
     * @param p Aim point
     * @param current_state Current game state
     * @return Resulting state after throw
     */
    [[nodiscard]] virtual State throw_at_sample(Vec2 p, State current_state) const = 0;
    
    /**
     * @brief Compute probability distribution of resulting states.
     * @param p Aim point
     * @param current_state Current game state
     * @return Vector of (resulting_state, probability) pairs
     */
    [[nodiscard]] virtual std::vector<std::pair<State, double>> throw_at(Vec2 p, State current_state) const = 0;
};

/**
 * @brief Game variant where any hit that reaches exactly zero wins.
 * @ingroup game
 * 
 * Simpler rules: just reduce score by hit value. Busts (going below zero)
 * leave the state unchanged.
 */
class GameFinishOnAny : public Game {
    State handle_throw(State current_state, HitData hit_data) const;
public:
    using Game::Game;
    [[nodiscard]] State throw_at_sample(Vec2 p, State current_state) const;
    [[nodiscard]] std::vector<std::pair<State, double>> throw_at(Vec2 p, State current_state) const;
};

/**
 * @brief Standard darts rules: must finish on a double.
 * @ingroup game
 * 
 * To win, the final hit must:
 * - Be a double (hit type)
 * - Reduce score to exactly zero
 * 
 * Hitting exactly zero with non-double or reaching score 1 busts.
 */
class GameFinishOnDouble : public Game {
private:
    State handle_throw(State current_state, HitData hit_data) const;
public:
    using Game::Game;
    [[nodiscard]] State throw_at_sample(Vec2 p, State current_state) const;
    [[nodiscard]] std::vector<std::pair<State, double>> throw_at(Vec2 p, State current_state) const;
};

/**
 * @ingroup game
 * @brief Information about a dart hit.
 */
struct HitData {
    enum class Type {
        NORMAL,  ///< Regular hit
        DOUBLE,  ///< Double ring
        TREBLE   ///< Treble ring
    };

    HitData() = default;
    HitData(Type type, Game::StateDifference diff) : type(type), diff(diff) {}
    Type type;                         ///< Hit type
    Game::StateDifference diff;        ///< Score change (negative reduces remaining)

    bool operator<(const HitData& other) const {
        if (type != other.type) {
            return type < other.type;
        }
        return diff < other.diff;
    }
};

/**
 * @ingroup game
 * @brief Dartboard target definition.
 *
 * Consists of multiple beds (scoring regions), each a polygon with
 * associated hit data (score value and type).
 *
 * Example usage:
 * @code
 * Target board("standard_board.txt");
 * HitData hit = board.after_hit(Vec2{15.2, 3.1});
 * // Returns hit type and score for that point
 * @endcode
 */
class Target {
    class Bed;

    std::vector<Bed> beds_;
    static constexpr int MISS_STATE_DIFF_ = 0;
public:
    Target() = default;
    Target(const std::vector<Bed>& beds);
    
    /** @brief Load target from input stream. */
    explicit Target(std::istream &input);
    
    /** @brief Load target from file. */
    explicit Target(const std::string &filename);
    
    void import(std::istream &input);
    void import(const std::string &filename);

    /**
     * @brief Determine what was hit at position p.
     * @param p Hit position
     * @return HitData with type and score, or miss (diff=0) if outside all beds
     */
    [[nodiscard]] HitData after_hit(Vec2 p) const;
    
    [[nodiscard]] const std::vector<Bed>& get_beds() const {
        return beds_;
    }
};

/**
 * @brief A single scoring region (bed) on the dartboard.
 * @ingroup game
 * 
 * Consists of a polygonal shape and the hit data returned when that
 * region is hit.
 */
class Target::Bed {
private:
    Polygon shape_;
    HitData after_hit_data_;

public:
    Bed() = default;
    Bed(const Polygon& shape, Game::StateDifference diff, HitData::Type type = HitData::Type::NORMAL) :
        shape_(shape), after_hit_data_(type, diff) {};
    Bed(const Polygon& shape, HitData after_hit_data) : shape_(shape), after_hit_data_(after_hit_data) {};
    void import(std::istream &input);

    /** @brief Check if point p is inside this bed. */
    bool inside(Vec2 p) const;
    
    /** @brief Get the hit data for this bed. */
    [[nodiscard]] HitData after_hit() const;
    
    [[nodiscard]] const Polygon& get_shape() const {
        return shape_;
    }
};

#endif