#pragma once

#include <magic_enum/magic_enum.hpp>

#include "core/util/export.h"

namespace algos::rfd {

enum class DESBORDANTE_EXPORT RngEngine : char {
    kMt19937 = 0,  // Mersenne Twister 19937
    kPcg32,        // PCG-32 (fast, small state)
    kXoshiro256,   // xoshiro256** (very fast, small state)
    kMinstdRand,   // std::minstd_rand (tiny 32-bit LCG, cheapest)
};

}  // namespace algos::rfd
