#include "Game.h"
#include "Geometry.h"
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>
#include <unordered_map>
#include <map>


// TODO remove dedup
Game::HitDistribution Game::throw_at_distribution_(Vec2 p) const {
    if (throw_at_cache_.contains(p)) {
        return throw_at_cache_.at(p);
    }

    std::map<HitData, double> result;
    double total_probability = 0.0;
    
    for (const auto& region : target_.get_beds()) {
        double probability = distribution_.integrate_probability(
            region.get_shape(),
            p
        );
        total_probability += probability;
        HitData diff = region.after_hit();
        result[diff] += probability;
    }
    
    result[HitData(HitData::Type::NORMAL, 0)] += 1.0 - total_probability;

    throw_at_cache_[p] = std::vector<std::pair<HitData, double>>(result.begin(), result.end());
    return throw_at_cache_.at(p);
}

Game::Game(const Target& target, const Distribution& distribution) 
    : target_(target), distribution_(distribution) {}

Game::Bounds Game::get_target_bounds() const {
    if (target_bounds_.min.x != std::numeric_limits<double>::max()) {
        return target_bounds_;
    }

    for (const auto& bed : target_.get_beds()) {
        for (const auto& vertex : bed.get_shape().get_vertices()) {
            if (vertex.x < target_bounds_.min.x) 
                target_bounds_.min.x = vertex.x;
            if (vertex.y < target_bounds_.min.y) 
                target_bounds_.min.y = vertex.y;
            if (vertex.x > target_bounds_.max.x) 
                target_bounds_.max.x = vertex.x;
            if (vertex.y > target_bounds_.max.y) 
                target_bounds_.max.y = vertex.y;
        }
    }

    double scale = target_bounds_.max.x - target_bounds_.min.x;
    target_bounds_.min.x -= scale * 0.1;
    target_bounds_.max.x += scale * 0.1;
    scale = target_bounds_.max.y - target_bounds_.min.y;
    target_bounds_.min.y -= scale * 0.1;
    target_bounds_.max.y += scale * 0.1;

    return target_bounds_;
}

Game::State GameFinishOnAny::handle_throw(State current_state, HitData hit_data) const {
    if (hit_data.diff + static_cast<StateDifference>(current_state) < 0) {
        return current_state;
    }
    return current_state + hit_data.diff;
}

Game::State GameFinishOnAny::throw_at_sample(Vec2 p, State current_state) const {
    Vec2 sample = distribution_.sample() + p;
    HitData hit_data = target_.after_hit(sample);

    return handle_throw(current_state, hit_data);
}

std::vector<std::pair<Game::State, double>> GameFinishOnAny::throw_at(Vec2 p, State current_state) const {
    auto diff_distribution = throw_at_distribution_(p);
    std::vector<std::pair<Game::State, double>> result;
    
    for (const auto& [hit_data, probability] : diff_distribution) {
        result.emplace_back(handle_throw(current_state, hit_data), probability);
    }
    
    return result;
}


Game::State GameFinishOnDouble::handle_throw(State current_state, HitData hit_data) const {
    if (hit_data.diff + static_cast<StateDifference>(current_state) == 0) {
        if (hit_data.type == HitData::Type::DOUBLE) {
            return 0;
        }
        else return current_state;
    }
    if (hit_data.diff + static_cast<StateDifference>(current_state) < 0) {
        return current_state;
    }
    return current_state + hit_data.diff;
}

Game::State GameFinishOnDouble::throw_at_sample(Vec2 p, State current_state) const {
    Vec2 sample = distribution_.sample() + p;
    HitData hit_data = target_.after_hit(sample);

    return handle_throw(current_state, hit_data);
}

std::vector<std::pair<Game::State, double>> GameFinishOnDouble::throw_at(Vec2 p, State current_state) const {
    auto diff_distribution = throw_at_distribution_(p);
    std::vector<std::pair<Game::State, double>> result;
    
    for (const auto& [hit_data, probability] : diff_distribution) {
        result.emplace_back(handle_throw(current_state, hit_data), probability);
    }
    
    return result;
}




bool Target::Bed::inside(Vec2 p) const {
    return shape_.contains(p);
}

HitData Target::Bed::after_hit() const {
    return after_hit_data_;
}

Target::Target(const std::vector<Bed>& beds) : beds_(beds) {}

Target::Target(std::istream &input) {
    import(input);
}

Target::Target(const std::string &filename) {
    import(filename);
}

HitData Target::after_hit(Vec2 p) const {
    for (const auto& bed : beds_) {
        if (bed.inside(p)) {
            return bed.after_hit();
        }
    }
    return HitData(HitData::Type::NORMAL, MISS_STATE_DIFF_);
}

void Target::Bed::import(std::istream &input) {
    int score;
    input >> score;
    
    int num_points;
    input >> num_points;
    
    std::string _;
    input >> _; // ignore color
    
    std::string type_str;
    input >> type_str;
    HitData::Type type = HitData::Type::NORMAL;
    if (type_str == "double") type = HitData::Type::DOUBLE;
    else if (type_str == "treble") type = HitData::Type::TREBLE;

    after_hit_data_ = HitData(type, -score);
    
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
