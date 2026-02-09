include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(darts_tests
  test_distributions.cpp
)

target_link_libraries(darts_tests PRIVATE
  darts_core
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(darts_tests)
