#include "Game.h"
#include "Geometry.h"
#include <csignal>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <utility>
#include <map>

Game::Game(const Target& target, const Distribution& distribution) 
    : target(target), distribution(distribution) {}

Game::State Game::throw_at_sample(Point p, State current_state) const {
    Point sample = distribution.sample() + static_cast<PointDifference>(p);
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

std::pair<Point, Point> Game::get_target_bounds() const {
    if (target_bounds.first.x != std::numeric_limits<double>::max()) {
        return target_bounds;
    }

    for (const auto& bed : target.get_beds()) {
        for (const auto& vertex : bed.get_shape().get_vertices()) {
            if (vertex.x < target_bounds.first.x) 
                const_cast<Point&>(target_bounds.first).x = vertex.x;
            if (vertex.y < target_bounds.first.y) 
                const_cast<Point&>(target_bounds.first).y = vertex.y;
            if (vertex.x > target_bounds.second.x) 
                const_cast<Point&>(target_bounds.second).x = vertex.x;
            if (vertex.y > target_bounds.second.y) 
                const_cast<Point&>(target_bounds.second).y = vertex.y;
        }
    }

    return target_bounds;
}

Target::Bed::Bed(const Polygon& shape, Game::StateDifference diff) : shape(shape), diff(diff) {}


bool Target::Bed::inside(Point p) const {
    return shape.contains(p);
}

Game::StateDifference Target::Bed::after_hit() const {
    return diff;
}

Target::Target(const std::vector<Bed>& beds) : beds(beds) {}

Target::Target(std::istream &input) {
    import(input);
}

Target::Target(const std::string &filename) {
    import(filename);
}

Game::StateDifference Target::after_hit(Point p) const {
    for (const auto& bed : beds) {
        if (bed.inside(p)) {
            return bed.after_hit();
        }
    }
    return MISS_STATE_DIFF;
}

void Target::Bed::import(std::istream &input) {
    int score;
    input >> score;
    diff = -score;

    int num_points;
    input >> num_points;
    std::vector<Point> vertices(num_points);
    for (int i = 0; i < num_points; ++i) {
        input >> vertices[i].x >> vertices[i].y;
    }
    shape.set_vertices(std::move(vertices));
}

void Target::import(std::istream &input) {
    int num_beds;
    input >> num_beds;
    beds.clear();
    beds.resize(num_beds);
    for (int i = 0; i < num_beds; ++i) {
        beds[i].import(input);
    }
}

void Target::import(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open target file: " + filename);
    }
    import(file);
}
