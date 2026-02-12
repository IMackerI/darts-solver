# Library with core logic
add_library(darts_core STATIC
  Geometry.cpp
  Game.cpp
  Distribution.cpp
  Solver.cpp
)

target_include_directories(darts_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(darts_core PUBLIC cxx_std_23)

if (MSVC)
  target_compile_options(darts_core PRIVATE /W4)
else()
  target_compile_options(darts_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
