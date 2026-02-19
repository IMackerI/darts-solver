#include <emscripten/bind.h>
#include "../cpp/Geometry.h"
#include "../cpp/Game.h"
#include "../cpp/Distribution.h"
#include "../cpp/Solver.h"
#include <string>
#include <sstream>

using namespace emscripten;

// Helper to create Target from string content instead of file
Target* createTargetFromString(const std::string& content) {
    std::stringstream ss(content);
    return new Target(ss);
}

EMSCRIPTEN_BINDINGS(darts_module) {
    // Vec2 value type
    value_object<Vec2>("Vec2")
        .field("x", &Vec2::x)
        .field("y", &Vec2::y);
    
    // Game::Bounds struct
    value_object<Game::Bounds>("Bounds")
        .field("min", &Game::Bounds::min)
        .field("max", &Game::Bounds::max);
    
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
    
    // Concrete distribution classes - direct constructors with flat arrays
    class_<NormalDistributionRandom, base<NormalDistribution>>("NormalDistributionRandom")
        .constructor<std::vector<double>, Vec2, size_t>()
        .function("set_integration_precision", &NormalDistributionRandom::set_integration_precision);
    
    class_<NormalDistributionQuadrature, base<NormalDistribution>>("NormalDistributionQuadrature")
        .constructor<std::vector<double>, Vec2>();
    
    // Abstract Game base - expose inherited functions
    class_<Game>("Game")
        .function("throw_at_sample", &Game::throw_at_sample)
        .function("get_target_bounds", &Game::get_target_bounds);
    
    // Concrete game classes
    class_<GameFinishOnAny, base<Game>>("GameFinishOnAny")
        .constructor<const Target&, const Distribution&>();
    
    class_<GameFinishOnDouble, base<Game>>("GameFinishOnDouble")
        .constructor<const Target&, const Distribution&>();
    
    // Solver
    class_<Solver>("Solver")
        .constructor<const Game&, size_t>();
}