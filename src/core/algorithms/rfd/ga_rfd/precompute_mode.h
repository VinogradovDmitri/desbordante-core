#pragma once

#include <magic_enum/magic_enum.hpp>

#include "core/util/export.h"

namespace algos::rfd {

enum class DESBORDANTE_EXPORT PrecomputeMode : char {
    kAuto = 0,  // Precompute the support index only when it is cheap
    kOn,        // Always precompute (may take a lot of time and memory)
    kOff,       // Never precompute, always compute support on the fly
};

}  // namespace algos::rfd
