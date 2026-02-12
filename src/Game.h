#ifndef GAME_HEADER
#define GAME_HEADER

#include "Distribution.h"
#include "Geometry.h"
#include <utility>
#include <vector>
#include <string>
#include <limits>

class Target;

class Game {
public:
    using State = unsigned int;
    using StateDifference = int;
private:
    const Target& target;
    const Distribution& distribution;
    std::pair<Point, Point> target_bounds = {
        Point{std::numeric_limits<double>::max(), std::numeric_limits<double>::max()},
        Point{std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest()}
    };
public:
    Game(const Target& target, const Distribution& distribution);
    State throw_at_sample(Point p, State current_state) const;
    std::vector<std::pair<State, double>> throw_at(Point p, State current_state) const;
    std::pair<Point, Point> get_target_bounds() const;
};

class Target {
    class Bed;

    std::vector<Bed> beds;
    static constexpr int MISS_STATE_DIFF = 0;
public:
    Target() = default;
    Target(const std::vector<Bed>& beds);
    explicit Target(std::istream &input);
    explicit Target(const std::string &filename);
    void import(std::istream &input);
    void import(const std::string &filename);

    Game::StateDifference after_hit(Point p) const;
    const std::vector<Bed>& get_beds() const {
        return beds;
    }
};

class Target::Bed {
    Polygon shape;
    Game::StateDifference diff = 0;
public:
    Bed() = default;
    Bed(const Polygon& shape, Game::StateDifference diff);
    void import(std::istream &input);

    bool inside(Point p) const;
    Game::StateDifference after_hit() const;
    const Polygon& get_shape() const {
        return shape;
    }
};

#endif