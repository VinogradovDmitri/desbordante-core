#include "core/util/levenshtein_distance.h"

#include <algorithm>
#include <cassert>
#include <vector>

namespace util {

/* Levenshtein distance computation algorithm taken from
 * https://en.wikipedia.org/wiki/Levenshtein_distance
 */
unsigned LevenshteinDistance(std::string_view l, std::string_view r) {
    size_t r_size = r.size();
    std::vector<unsigned> v0(r_size + 1);
    std::vector<unsigned> v1(r_size + 1);

    for (unsigned i = 0; i != r_size + 1; ++i) {
        v0[i] = i;
    }

    for (unsigned i = 0; i != l.size(); ++i) {
        v1[0] = i + 1;

        for (unsigned j = 0; j != r.size(); ++j) {
            assert(j + 1 < v0.size());
            unsigned del_cost = v0[j + 1] + 1;
            unsigned insert_cost = v1[j] + 1;
            unsigned substitution_cost;
            if (l[i] == r[j]) {
                substitution_cost = v0[j];
            } else {
                substitution_cost = v0[j] + 1;
            }

            v1[j + 1] = std::min({del_cost, insert_cost, substitution_cost});
        }

        std::swap(v0, v1);
    }

    return v0.back();
}

// Threshold-pruned variant: returns the true distance when <= max_dist, otherwise
// max_dist + 1. Much faster when the true distance exceeds max_dist (the common
// case when a high minimum similarity is required), because the whole DP frontier
// exceeds max_dist within a few rows and we bail out early. Edit distance is
// non-decreasing in both string lengths, so once an entire row exceeds max_dist no
// later row can drop back below it.
unsigned LevenshteinDistance(std::string_view l, std::string_view r, unsigned max_dist) {
    if (max_dist == 0) {
        return l == r ? 0u : 1u;
    }
    // Operate on the shorter string as the DP column for a smaller working set.
    if (l.size() > r.size()) return LevenshteinDistance(r, l, max_dist);

    size_t const r_size = r.size();
    std::vector<unsigned> v0(r_size + 1);
    std::vector<unsigned> v1(r_size + 1);
    for (unsigned i = 0; i != r_size + 1; ++i) v0[i] = i;

    for (unsigned i = 0; i != l.size(); ++i) {
        v1[0] = i + 1;
        bool all_above = (i + 1) > max_dist;  // column 0 of this row
        for (unsigned j = 0; j != r_size; ++j) {
            unsigned const del_cost = v0[j + 1] + 1;
            unsigned const insert_cost = v1[j] + 1;
            unsigned const substitution_cost = (l[i] == r[j]) ? v0[j] : v0[j] + 1;
            v1[j + 1] = std::min({del_cost, insert_cost, substitution_cost});
            if (v1[j + 1] <= max_dist) all_above = false;
        }
        if (i != 0 && all_above) return max_dist + 1;
        std::swap(v0, v1);
    }

    unsigned const dist = v0.back();
    return dist <= max_dist ? dist : max_dist + 1;
}

}  // namespace util
