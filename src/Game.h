#ifndef TARGET_HEADER
#define TARGET_HEADER

#include "Distribution.h"
#include "Geometry.h"
#include <utility>
#include <vector>
#include <fstream>

class Target;

class Game {
public:
    using state = unsigned int;
    using state_diff = int;
private:
    const Target& target;
    const Distribution& distribution;
    public:
    Game(const Target& target, const Distribution& distribution);
    state throw_at_sample(Point p, state current_state) const;
    std::vector<std::pair<state, double>> throw_at(Point p, state current_state) const;
};

class Target {
    class Bed;

    std::vector<Bed> beds;
    static constexpr int MISS_STATE_DIFF = 0;
public:
    Target(const std::vector<Bed>& beds);
    void import(std::istream &input);
    void import(const std::string &filename);

    Game::state_diff after_hit(Point p) const;
    const std::vector<Bed>& get_beds() const {
        return beds;
    }
};

class Target::Bed {
    Polygon shape;
    Game::state_diff diff;
public:
    Bed(const Polygon& shape, Game::state_diff diff);
    void import(std::istream &input);

    bool inside(Point p) const;
    Game::state_diff after_hit() const;
    const Polygon& get_shape() const {
        return shape;
    }
};

#endif