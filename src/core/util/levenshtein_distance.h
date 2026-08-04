#pragma once

#include <string_view>

namespace util {

unsigned LevenshteinDistance(std::string_view l, std::string_view r);

// Threshold-pruned variant: returns the true distance when <= max_dist, otherwise
// max_dist + 1. Much faster when the true distance exceeds max_dist.
unsigned LevenshteinDistance(std::string_view l, std::string_view r, unsigned max_dist);

}  // namespace util
