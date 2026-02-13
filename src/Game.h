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

struct HitData;

class Game {
public:
    using State = unsigned int;
    using StateDifference = int;
    using HitDistribution = std::vector<std::pair<HitData, double>>;
protected:
    const Target& target_;
    const Distribution& distribution_;
    
    mutable std::unordered_map<Vec2, HitDistribution> throw_at_cache_;
    mutable std::pair<Vec2, Vec2> target_bounds_ = {
        Vec2{std::numeric_limits<double>::max(), std::numeric_limits<double>::max()},
        Vec2{std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest()}
    };

    HitDistribution throw_at_distribution_(Vec2 p) const;
public:
    Game(const Target& target, const Distribution& distribution);
    [[nodiscard]] std::pair<Vec2, Vec2> get_target_bounds() const;
    virtual ~Game() = default;
    [[nodiscard]] virtual State throw_at_sample(Vec2 p, State current_state) const = 0;
    [[nodiscard]] virtual std::vector<std::pair<State, double>> throw_at(Vec2 p, State current_state) const = 0;
};

class GameFinishOnAny : public Game {
    State handle_throw(State current_state, HitData hit_data) const;
public:
    using Game::Game;
    [[nodiscard]] State throw_at_sample(Vec2 p, State current_state) const;
    [[nodiscard]] std::vector<std::pair<State, double>> throw_at(Vec2 p, State current_state) const;
};

class GameFinishOnDouble : public Game {
private:
    State handle_throw(State current_state, HitData hit_data) const;
public:
    using Game::Game;
    [[nodiscard]] State throw_at_sample(Vec2 p, State current_state) const;
    [[nodiscard]] std::vector<std::pair<State, double>> throw_at(Vec2 p, State current_state) const;
};

struct HitData {
    enum class Type {
        NORMAL,
        DOUBLE,
        TREBLE
    };

    HitData() = default;
    HitData(Type type, Game::StateDifference diff) : type(type), diff(diff) {}
    Type type;
    Game::StateDifference diff;

    bool operator<(const HitData& other) const {
        if (type != other.type) {
            return type < other.type;
        }
        return diff < other.diff;
    }
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

    [[nodiscard]] HitData after_hit(Vec2 p) const;
    [[nodiscard]] const std::vector<Bed>& get_beds() const {
        return beds_;
    }
};

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

    bool inside(Vec2 p) const;
    [[nodiscard]] HitData after_hit() const;
    [[nodiscard]] const Polygon& get_shape() const {
        return shape_;
    }
};

#endif