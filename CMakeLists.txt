cmake_minimum_required(VERSION 3.20)
project(darts_solver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


add_subdirectory(src)

# Console application
add_executable(darts_solver main.cpp)
target_link_libraries(darts_solver PRIVATE darts_core)

enable_testing()
add_subdirectory(tests)