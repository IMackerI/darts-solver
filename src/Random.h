#ifndef SEED_HEADER
#define SEED_HEADER

#include <random>
constexpr unsigned int SEED = 123456789;
std::mt19937 random_engine(SEED);

#endif