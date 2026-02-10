#include "Game.h"
#include "Geometry.h"
#include <csignal>
#include <vector>
#include <utility>
#include <map>

Game::Game(const Target& target, const Distribution& distribution) 
    : target(target), distribution(distribution) {}

Game::State Game::throw_at_sample(Point p, State current_state) const {
    Point sample = distribution.sample();
    StateDifference diff = target.after_hit(sample);

    if (diff + current_state < 0) return current_state;
    else return current_state + diff;
}

std::vector<std::pair<Game::State, double>> Game::throw_at(Point p, State current_state) const {
    std::map<Game::State, double> result;

    for (auto&& region : target.get_beds()) {
        double probability = distribution.integrate_probability(
            region.get_shape(),
            static_cast<PointDifference>(p)
        );
        StateDifference diff = region.after_hit();

        if (diff + current_state < 0) result[current_state] += probability;
        else result[current_state + diff] += probability;
    }

    return std::vector<std::pair<Game::State, double>>(result.begin(), result.end());
}

Target::Bed::Bed(const Polygon& shape, Game::StateDifference diff) : shape(shape), diff(diff) {}


bool Target::Bed::inside(Point p) const {
    return shape.contains(p);
}

Game::StateDifference Target::Bed::after_hit() const {
    return diff;
}

Target::Target(const std::vector<Bed>& beds) : beds(beds) {}

Game::StateDifference Target::after_hit(Point p) const {
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
