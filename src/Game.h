#ifndef GAME_HEADER
#define GAME_HEADER

#include "Distribution.h"
#include "Geometry.h"
#include <unordered_map>
#include <utility>
#include <vector>
#include <string>
#include <limits>

class Target;

class Game {
public:
    using State = unsigned int;
    using StateDifference = int;
    using StateDiffDistribution = std::vector<std::pair<StateDifference, double>>;
private:
    const Target& target_;
    const Distribution& distribution_;
    
    mutable std::unordered_map<Vec2, StateDiffDistribution> throw_at_cache_;
    mutable std::pair<Vec2, Vec2> target_bounds_ = {
        Vec2{std::numeric_limits<double>::max(), std::numeric_limits<double>::max()},
        Vec2{std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest()}
    };

    StateDiffDistribution throw_at_distribution_(Vec2 p) const;
public:
    Game(const Target& target, const Distribution& distribution);
    
    [[nodiscard]] State throw_at_sample(Vec2 p, State current_state) const;
    [[nodiscard]] std::vector<std::pair<State, double>> throw_at(Vec2 p, State current_state) const;
    [[nodiscard]] std::pair<Vec2, Vec2> get_target_bounds() const;
};

class Target {
    class Bed;

    std::vector<Bed> beds_;
    static constexpr int MISS_STATE_DIFF_ = 0;
public:
    Target() = default;
    Target(const std::vector<Bed>& beds);
    explicit Target(std::istream &input);
    explicit Target(const std::string &filename);
    void import(std::istream &input);
    void import(const std::string &filename);

    [[nodiscard]] Game::StateDifference after_hit(Vec2 p) const;
    [[nodiscard]] const std::vector<Bed>& get_beds() const {
        return beds_;
    }
};

class Target::Bed {
    Polygon shape_;
    Game::StateDifference diff_ = 0;
public:
    Bed() = default;
    Bed(const Polygon& shape, Game::StateDifference diff);
    void import(std::istream &input);

    bool inside(Vec2 p) const;
    [[nodiscard]] Game::StateDifference after_hit() const;
    [[nodiscard]] const Polygon& get_shape() const {
        return shape_;
    }
};

#endif