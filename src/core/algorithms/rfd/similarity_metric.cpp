#include "core/algorithms/rfd/similarity_metric.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "core/util/levenshtein_distance.h"

namespace algos::rfd {

FunctionSimilarityMetric::FunctionSimilarityMetric(Func func) : func_(std::move(func)) {}

double FunctionSimilarityMetric::Compare(std::string const& a, std::string const& b) const {
    return func_(a, b);
}

namespace {
double LevenshteinSimilarity(std::string const& a, std::string const& b) {
    size_t n = a.size(), m = b.size();
    if (n == 0 && m == 0) return 1.0;
    if (n == 0 || m == 0) return 0.0;
    std::vector<size_t> dp(m + 1);
    for (size_t j = 0; j <= m; j++) dp[j] = j;
    for (size_t i = 1; i <= n; i++) {
        size_t prev = i;
        for (size_t j = 1; j <= m; j++) {
            size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            size_t cur = std::min({dp[j] + 1, prev + 1, dp[j - 1] + cost});
            dp[j - 1] = prev;
            prev = cur;
        }
        dp[m] = prev;
    }
    double max_len = std::max(n, m);
    return 1.0 - static_cast<double>(dp[m]) / max_len;
}

// Returns whether the Levenshtein similarity of (a, b) is >= min_sim, using a
// threshold-pruned distance that bails out early once the distance cannot reach
// the required bound (the usual case for a high minimum similarity).
bool LevenshteinSatisfies(std::string const& a, std::string const& b, double min_sim) {
    size_t const n = a.size(), m = b.size();
    if (n == 0 && m == 0) return 1.0 >= min_sim;
    if (n == 0 || m == 0) return 0.0 >= min_sim;
    if (std::isnan(min_sim)) return false;
    double const max_len = static_cast<double>(std::max(n, m));
    if (min_sim <= 0.0) return true;  // any non-empty pair has similarity >= 0
    if (min_sim > 1.0) return false;
    unsigned const max_dist = static_cast<unsigned>(std::ceil((1.0 - min_sim) * max_len));
    unsigned const dist = util::LevenshteinDistance(a, b, max_dist);
    if (dist > max_dist) return false;
    return (1.0 - static_cast<double>(dist) / max_len) >= min_sim;
}
}  // namespace

std::shared_ptr<SimilarityMetric> LevenshteinMetric() {
    // Dedicated subclass so GA-RFD can use the integer-id fast path when the
    // threshold is strict (min_sim >= 1.0) and the threshold-pruned distance
    // otherwise.
    class LevenshteinSim final : public FunctionSimilarityMetric {
    public:
        LevenshteinSim() : FunctionSimilarityMetric(&LevenshteinSimilarity) {}

        bool IsExactSimilarity() const override {
            return true;
        }

        bool Satisfies(std::string const& a, std::string const& b, double min_sim) const override {
            return LevenshteinSatisfies(a, b, min_sim);
        }
    };

    return std::make_shared<LevenshteinSim>();
}

std::shared_ptr<SimilarityMetric> EqualityMetric() {
    // Dedicated subclass so GA-RFD can detect exact-equality metrics and intern
    // column values to integer ids for a much faster pair-comparison loop.
    class EqualitySim final : public FunctionSimilarityMetric {
    public:
        EqualitySim()
            : FunctionSimilarityMetric([](std::string const& a, std::string const& b) {
                  return a == b ? 1.0 : 0.0;
              }) {}

        bool IsEquality() const override {
            return true;
        }

        bool IsExactSimilarity() const override {
            return true;
        }
    };

    return std::make_shared<EqualitySim>();
}

std::shared_ptr<SimilarityMetric> AbsoluteDifferenceMetric() {
    // Numeric metric: the per-pair compare is done on pre-parsed doubles.
    class AbsDiffSim final : public FunctionSimilarityMetric {
    public:
        AbsDiffSim()
            : FunctionSimilarityMetric([](std::string const& a, std::string const& b) {
                  try {
                      double x = std::stod(a);
                      double y = std::stod(b);
                      if (std::isnan(x) || std::isnan(y) || std::isinf(x) || std::isinf(y)) return 0.0;
                      double abs_diff = std::abs(x - y);
                      double max_abs = std::max(std::abs(x), std::abs(y));
                      if (max_abs == 0.0) return 1.0;
                      double similarity = 1.0 - abs_diff / max_abs;
                      return std::max(0.0, similarity);
                  } catch (...) {
                      return 0.0;
                  }
              }) {}

        bool IsNumeric() const override {
            return true;
        }

        bool IsExactSimilarity() const override {
            return true;
        }

        bool NumericSatisfies(double x, double y, double min_sim) const override {
            // Unparseable cells yield 0.0 from Compare (see the string lambda
            // above), so NaN must map to the same 0.0 >= min_sim result.
            if (std::isnan(x) || std::isnan(y)) return 0.0 >= min_sim;
            double const abs_diff = std::abs(x - y);
            double const max_abs = std::max(std::abs(x), std::abs(y));
            if (max_abs == 0.0) return 1.0 >= min_sim;
            double const similarity = 1.0 - abs_diff / max_abs;
            return std::max(0.0, similarity) >= min_sim;
        }
    };

    return std::make_shared<AbsDiffSim>();
}

std::shared_ptr<SimilarityMetric> AbsoluteThresholdMetric(double diff) {
    class AbsThresholdSim final : public FunctionSimilarityMetric {
    public:
        explicit AbsThresholdSim(double d)
            : FunctionSimilarityMetric([d](std::string const& a, std::string const& b) {
                  try {
                      double x = std::stod(a);
                      double y = std::stod(b);
                      if (std::isnan(x) || std::isnan(y) || std::isinf(x) || std::isinf(y)) return 0.0;
                      return (std::abs(x - y) <= d) ? 1.0 : 0.0;
                  } catch (...) {
                      return 0.0;
                  }
              }),
              diff_(d) {}

        bool IsNumeric() const override {
            return true;
        }

        bool NumericSatisfies(double x, double y, double min_sim) const override {
            // Unparseable cells yield 0.0 from Compare, so NaN maps to 0.0 >= min_sim.
            if (std::isnan(x) || std::isnan(y)) return 0.0 >= min_sim;
            bool const similar = std::abs(x - y) <= diff_;
            return (similar ? 1.0 : 0.0) >= min_sim;
        }

    private:
        double diff_;
    };

    return std::make_shared<AbsThresholdSim>(diff);
}

}  // namespace algos::rfd
