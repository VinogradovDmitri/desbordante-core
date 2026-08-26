#pragma once

#include <functional>
#include <memory>
#include <string>

#include "core/util/export.h"

namespace algos::rfd {

class DESBORDANTE_EXPORT SimilarityMetric {
public:
    virtual ~SimilarityMetric() = default;
    virtual double Compare(std::string const& a, std::string const& b) const = 0;

    // Returns whether Compare(a, b) >= min_sim. The default just calls Compare, but
    // metrics may override it with a short-circuiting implementation (e.g. a
    // threshold-pruned edit distance) that avoids computing the full similarity.
    virtual bool Satisfies(std::string const& a, std::string const& b, double min_sim) const {
        return Compare(a, b) >= min_sim;
    }

    // True only for exact-equality metrics, which allow the GA-RFD bitset build to
    // compare interned integer ids instead of strings (a large speedup).
    virtual bool IsEquality() const {
        return false;
    }

    // True for metrics whose similarity is exactly 1.0 iff the two strings are
    // identical (equality, Levenshtein, absolute difference). Lets GA-RFD use the
    // integer-id fast path whenever the similarity threshold is >= 1.0.
    virtual bool IsExactSimilarity() const {
        return false;
    }

    // True for purely numeric metrics (absolute difference, absolute threshold),
    // whose per-pair comparison can be done on pre-parsed doubles.
    virtual bool IsNumeric() const {
        return false;
    }

    // For numeric metrics only: returns whether two parsed doubles satisfy
    // Compare(a,b) >= min_sim. Must reproduce the exact semantics of Compare
    // (including clamping and unparseable handling).
    virtual bool NumericSatisfies(double /*a*/, double /*b*/, double /*min_sim*/) const {
        return false;
    }
};

class FunctionSimilarityMetric : public SimilarityMetric {
public:
    using Func = std::function<double(std::string const&, std::string const&)>;
    explicit FunctionSimilarityMetric(Func func);
    double Compare(std::string const& a, std::string const& b) const override;

private:
    Func func_;
};

std::shared_ptr<SimilarityMetric> LevenshteinMetric();
std::shared_ptr<SimilarityMetric> EqualityMetric();
std::shared_ptr<SimilarityMetric> AbsoluteDifferenceMetric();
std::shared_ptr<SimilarityMetric> AbsoluteThresholdMetric(double diff);

}  // namespace algos::rfd
