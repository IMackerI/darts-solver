#include "Game.h"
#include "Geometry.h"
#include <csignal>
#include <vector>
#include <utility>
#include <map>

Game::Game(const Target& target, const Distribution& distribution) : target(target), distribution(distribution) {}

Game::state Game::throw_at_sample(Point p, state current_state) const {
    Point sample = distribution.sample();
    state_diff diff = target.after_hit(sample);

    if (diff + current_state < 0) return current_state;
    else return current_state + diff;
}

std::vector<std::pair<Game::state, double>> Game::throw_at(Point p, state current_state) const {
    std::map<Game::state, double> result;

    for (auto&& region : target.get_beds()) {
        double probability = distribution.integrate_probability(region.get_shape());
        state_diff diff = region.after_hit();

        if (diff + current_state < 0) result[current_state] += probability;
        else result[current_state + diff] += probability;
    }

    return std::vector<std::pair<Game::state, double>>(result.begin(), result.end());
}

Target::Bed::Bed(const Polygon& shape, Game::state_diff diff) : shape(shape), diff(diff) {}


bool Target::Bed::inside(Point p) const {
    return shape.contains(p);
}

Game::state_diff Target::Bed::after_hit() const {
    return diff;
}

Target::Target(const std::vector<Bed>& beds) : beds(beds) {}

Game::state_diff Target::after_hit(Point p) const {
    for (const auto& bed : beds) {
        if (bed.inside(p)) {
            return bed.after_hit();
        }
    }
    return MISS_STATE_DIFF;
}

void Target::Bed::import(std::istream &input) {
    
}

void Target::import(std::istream &input) {
    
}

void Target::import(const std::string &filename) {
    
}
