#include <emscripten/bind.h>
#include "../cpp/Geometry.h"
#include "../cpp/Game.h"
#include <string>

using namespace emscripten;

// Simple example function
int add(int a, int b) {
    return a + b;
}

// Example function using the darts library
std::string getCircleInfo(double radius) {
    return "Circle with radius: " + std::to_string(radius);
}

// Embind bindings - expose C++ functions to JavaScript
EMSCRIPTEN_BINDINGS(darts_module) {
    function("add", &add);
    function("getCircleInfo", &getCircleInfo);
}