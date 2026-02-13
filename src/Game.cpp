#include "Game.h"
#include "Geometry.h"
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>
#include <unordered_map>

Game::StateDiffDistribution Game::throw_at_distribution_(Vec2 p) const {
    if (throw_at_cache_.contains(p)) {
        return throw_at_cache_.at(p);
    }

    std::unordered_map<Game::StateDifference, double> result;
    double total_probability = 0;
    for (auto&& region : target_.get_beds()) {
        double probability = distribution_.integrate_probability(
            region.get_shape(),
            static_cast<Vec2>(p)
        );
        total_probability += probability;
        StateDifference diff = region.after_hit();

        result[diff] += probability;
    }
    result[0] += 1 - total_probability;

    throw_at_cache_[p] = std::vector<std::pair<Game::StateDifference, double>>(result.begin(), result.end());
    return throw_at_cache_.at(p);
}

Game::Game(const Target& target, const Distribution& distribution) 
    : target_(target), distribution_(distribution) {}

Game::State Game::throw_at_sample(Vec2 p, State current_state) const {
    Vec2 sample = distribution_.sample() + static_cast<Vec2>(p);
    StateDifference diff = target_.after_hit(sample);

    if (diff + static_cast<StateDifference>(current_state) < 0) return current_state;
    else return current_state + diff;
}

std::vector<std::pair<Game::State, double>> Game::throw_at(Vec2 p, State current_state) const {
    auto diff_distribution = throw_at_distribution_(p);
    std::vector<std::pair<Game::State, double>> result;
    for (const auto& [diff, probability] : diff_distribution) {
        if(diff + static_cast<StateDifference>(current_state) < 0) {
            result.emplace_back(current_state, probability);
        } else {
            result.emplace_back(current_state + diff, probability);
        }
    }
    return result;
}

std::pair<Vec2, Vec2> Game::get_target_bounds() const {
    if (target_bounds_.first.x != std::numeric_limits<double>::max()) {
        return target_bounds_;
    }

    for (const auto& bed : target_.get_beds()) {
        for (const auto& vertex : bed.get_shape().get_vertices()) {
            if (vertex.x < target_bounds_.first.x) 
                const_cast<Vec2&>(target_bounds_.first).x = vertex.x;
            if (vertex.y < target_bounds_.first.y) 
                const_cast<Vec2&>(target_bounds_.first).y = vertex.y;
            if (vertex.x > target_bounds_.second.x) 
                const_cast<Vec2&>(target_bounds_.second).x = vertex.x;
            if (vertex.y > target_bounds_.second.y) 
                const_cast<Vec2&>(target_bounds_.second).y = vertex.y;
        }
    }

    return target_bounds_;
}

Target::Bed::Bed(const Polygon& shape, Game::StateDifference diff) : shape_(shape), diff_(diff) {}


bool Target::Bed::inside(Vec2 p) const {
    return shape_.contains(p);
}

Game::StateDifference Target::Bed::after_hit() const {
    return diff_;
}

Target::Target(const std::vector<Bed>& beds) : beds_(beds) {}

Target::Target(std::istream &input) {
    import(input);
}

Target::Target(const std::string &filename) {
    import(filename);
}

Game::StateDifference Target::after_hit(Vec2 p) const {
    for (const auto& bed : beds_) {
        if (bed.inside(p)) {
            return bed.after_hit();
        }
    }
    return MISS_STATE_DIFF_;
}

void Target::Bed::import(std::istream &input) {
    int score;
    input >> score;
    diff_ = -score;

    int num_points;
    input >> num_points;
    std::string _;
    input >> _; // ignore color
    std::vector<Vec2> vertices(num_points);
    for (int i = 0; i < num_points; ++i) {
        input >> vertices[i].x >> vertices[i].y;
    }
    shape_.set_vertices(std::move(vertices));
}

void Target::import(std::istream &input) {
    int num_beds;
    input >> num_beds;
    beds_.clear();
    beds_.resize(num_beds);
    for (int i = 0; i < num_beds; ++i) {
        beds_[i].import(input);
    }
}

void Target::import(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open target file: " + filename);
    }
    import(file);
}
