# WebAssembly target - only build when using Emscripten
if(EMSCRIPTEN)
  add_executable(darts_wasm glue.cpp)
  
  # Link with the core library
  target_link_libraries(darts_wasm PRIVATE darts_core)
  
  # Emscripten-specific flags
  target_link_options(darts_wasm PRIVATE
    "SHELL:--bind"                          # Enable Embind for C++/JS bindings
    "SHELL:-s MODULARIZE=1"                 # Export as a module
    "SHELL:-s EXPORT_NAME=DartsModule"      # Name of the exported module
    "SHELL:-s ENVIRONMENT=web"              # Target web environment
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"        # Allow memory to grow
    "SHELL:-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']"  # Export runtime methods
  )
  
  # Set output directory to web folder
  set_target_properties(darts_wasm PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/src/web"
  )
else()
  message(STATUS "Skipping WebAssembly build (use emcmake to build)")
endif()