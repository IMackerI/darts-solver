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

// Helper struct for Solver::solve return value
struct SolveResult {
    double expected_throws;
    Vec2 optimal_aim;
};

// Helper function to convert JavaScript array to covariance matrix
NormalDistribution::covariance js_to_covariance(const val& js_array) {
    NormalDistribution::covariance cov;
    // Expecting flat array: [row0_col0, row0_col1, row1_col0, row1_col1]
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            cov[i][j] = js_array[i * 2 + j].as<double>();
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

// Wrapper for Solver::solve to return a simple struct
SolveResult solverSolve(Solver& solver, Game::State state) {
    auto [score, aim] = solver.solve(state);
    return SolveResult{score, aim};
}

EMSCRIPTEN_BINDINGS(darts_module) {
    // Register vector types
    register_vector<double>("VectorDouble");
    register_vector<std::vector<double>>("VectorVectorDouble");
    
    // Vec2 value type
    value_object<Vec2>("Vec2")
        .field("x", &Vec2::x)
        .field("y", &Vec2::y);
    
    // Game::Bounds struct
    value_object<Game::Bounds>("Bounds")
        .field("min", &Game::Bounds::min)
        .field("max", &Game::Bounds::max);
    
    // SolveResult struct for Solver::solve return value
    value_object<SolveResult>("SolveResult")
        .field("expected_throws", &SolveResult::expected_throws)
        .field("optimal_aim", &SolveResult::optimal_aim);
    
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
        .function("throw_at_sample", &Game::throw_at_sample)
        .function("get_target_bounds", &Game::get_target_bounds);
    
    // Concrete game classes
    class_<GameFinishOnAny, base<Game>>("GameFinishOnAny")
        .constructor<const Target&, const Distribution&>();
    
    class_<GameFinishOnDouble, base<Game>>("GameFinishOnDouble")
        .constructor<const Target&, const Distribution&>();
    
    // Abstract Solver base - no constructor (pure virtual)
    class_<Solver>("Solver")
        .function("solve_aim", &Solver::solve_aim);
    
    // Concrete solver implementations
    class_<SolverMinThrows, base<Solver>>("SolverMinThrows")
        .constructor<const Game&, size_t>();
    
    class_<MaxPointsSolver, base<Solver>>("MaxPointsSolver")
        .constructor<const Game&, size_t>();
    
    // Wrapper function for Solver::solve (returns SolveResult instead of std::pair)
    function("solverSolve", &solverSolve);
    
    // HeatMapVisualizer
    class_<HeatMapVisualizer>("HeatMapVisualizer")
        .constructor<Solver&, size_t, size_t>()
        .function("heat_map", &HeatMapVisualizer::heat_map);
}