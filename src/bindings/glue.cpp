#include <emscripten/bind.h>
#include <emscripten/val.h>
#include "../cpp/Geometry.h"
#include "../cpp/Game.h"
#include "../cpp/Distribution.h"
#include "../cpp/Solver.h"
#include <string>
#include <array>
#include <sstream>

using namespace emscripten;

// Struct to return bounds (instead of std::pair)
struct Bounds {
    Vec2 min;
    Vec2 max;
};

// Helper function to convert JavaScript array to covariance matrix
NormalDistribution::covariance js_to_covariance(const val& js_array) {
    NormalDistribution::covariance cov;
    for (int i = 0; i < 2; ++i) {
        val row = js_array[i];
        for (int j = 0; j < 2; ++j) {
            cov[i][j] = row[j].as<double>();
        }
    }
    return cov;
}

// Wrapper constructors that take JavaScript arrays
NormalDistributionQuadrature* createNormalDistributionQuadrature(const val& cov_array, Vec2 mean) {
    auto cov = js_to_covariance(cov_array);
    return new NormalDistributionQuadrature(cov, mean);
}

NormalDistributionRandom* createNormalDistributionRandom(const val& cov_array, Vec2 mean, size_t num_samples) {
    auto cov = js_to_covariance(cov_array);
    return new NormalDistributionRandom(cov, mean, num_samples);
}

// Helper to create Target from string content instead of file
Target* createTargetFromString(const std::string& content) {
    std::stringstream ss(content);
    return new Target(ss);
}

// Wrapper for get_target_bounds that returns a simple struct instead of std::pair
Bounds getTargetBounds(const Game& game) {
    auto [min, max] = game.get_target_bounds();
    return Bounds{min, max};
}

EMSCRIPTEN_BINDINGS(darts_module) {
    // Vec2 value type with constructor
    value_object<Vec2>("Vec2")
        .field("x", &Vec2::x)
        .field("y", &Vec2::y);
    
    // Bounds struct for get_target_bounds return value
    value_object<Bounds>("Bounds")
        .field("min", &Bounds::min)
        .field("max", &Bounds::max);
    
    // Constructor function for Vec2
    function("createVec2", optional_override([](double x, double y) {
        return Vec2{x, y};
    }));
    
    // Target class - use wrapper that takes string content
    class_<Target>("Target")
        .constructor(&createTargetFromString, allow_raw_pointers())
        .class_function("fromString", &createTargetFromString, allow_raw_pointers());
    
    // Abstract Distribution base - expose inherited functions
    class_<Distribution>("Distribution")
        .function("sample", &Distribution::sample);
    
    // Abstract NormalDistribution - no constructor
    class_<NormalDistribution, base<Distribution>>("NormalDistribution");
    
    // Concrete distribution classes - use wrapper constructors
    class_<NormalDistributionRandom, base<NormalDistribution>>("NormalDistributionRandom")
        .constructor(&createNormalDistributionRandom, allow_raw_pointers())
        .function("set_integration_precision", &NormalDistributionRandom::set_integration_precision);
    
    class_<NormalDistributionQuadrature, base<NormalDistribution>>("NormalDistributionQuadrature")
        .constructor(&createNormalDistributionQuadrature, allow_raw_pointers());
    
    // Abstract Game base - expose inherited functions
    class_<Game>("Game")
        .function("throw_at_sample", &Game::throw_at_sample);
    
    // Wrapper function for get_target_bounds (can't bind std::pair directly)
    function("getTargetBounds", &getTargetBounds);
    
    // Concrete game classes
    class_<GameFinishOnAny, base<Game>>("GameFinishOnAny")
        .constructor<const Target&, const Distribution&>();
    
    class_<GameFinishOnDouble, base<Game>>("GameFinishOnDouble")
        .constructor<const Target&, const Distribution&>();
    
    // Solver
    class_<Solver>("Solver")
        .constructor<const Game&, size_t>();
}