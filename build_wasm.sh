#!/bin/bash

# Build script for WebAssembly version using Emscripten
# Prerequisites: Emscripten SDK must be installed and activated

echo "Building Darts Solver for WebAssembly..."

# Create build directory for wasm
mkdir -p build_wasm
cd build_wasm

# Configure with Emscripten
emcmake cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF

# Build
emmake make darts_wasm -j$(nproc)

echo ""
echo "Build complete! WebAssembly files generated in src/web/"
echo "To test, run: python3 -m http.server 8000"
echo "Then open: http://localhost:8000/src/web/"
