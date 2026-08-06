#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <list>
#include <memory>
#include <new>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#ifdef __linux__
#include <sys/mman.h>
#endif

#include "core/algorithms/algorithm.h"
#include "core/algorithms/rfd/ga_rfd/rng_engine.h"
#include "core/algorithms/rfd/ga_rfd/rng_wrapper.h"
#include "core/algorithms/rfd/rfd.h"
#include "core/algorithms/rfd/similarity_metric.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/config/thread_number/type.h"

namespace tests {
class GaRfdTester;
}

namespace algos::rfd {

class GaRfd final : public algos::Algorithm {
// Allocator that backs large, randomly-accessed buffers with huge pages when the
// kernel grants them, and otherwise falls back to a normal anonymous mapping
// advised with MADV_HUGEPAGE. All allocations go through mmap/munmap so the
// deallocate path is uniform. Safe to use as a stateless std::allocator.
template <typename T>
struct HugePageAllocator {
    using value_type = T;

    T* allocate(std::size_t n) {
        if (n == 0) return nullptr;
        std::size_t const bytes = n * sizeof(T);
#ifdef __linux__
        void* p = mmap(nullptr, bytes, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
        if (p == MAP_FAILED) {
            // Huge pages unavailable (or not permitted): fall back to a normal
            // anonymous mapping and advise the kernel to use huge pages for it.
            p = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (p != MAP_FAILED) madvise(p, bytes, MADV_HUGEPAGE);
        }
        if (p == MAP_FAILED) throw std::bad_alloc{};
        return static_cast<T*>(p);
#else
        return static_cast<T*>(::operator new(bytes));
#endif
    }

    void deallocate(T* p, std::size_t n) {
#ifdef __linux__
        if (p != nullptr) munmap(p, n * sizeof(T));
#else
        ::operator delete(p);
#endif
    }

    template <typename U>
    struct rebind {
        using other = HugePageAllocator<U>;
    };

    bool operator==(HugePageAllocator const&) const noexcept {
        return true;
    }
};


class GaRfd final : public algos::Algorithm {
private:
    struct Individual {
        uint32_t lhs_mask = 0;
        uint8_t rhs_index = 0;
        double confidence = 0.0;
        double support = 0.0;

        bool operator==(Individual const& other) const {
            return lhs_mask == other.lhs_mask && rhs_index == other.rhs_index;
        }
    };

    struct IndividualHash {
        std::size_t operator()(Individual const& ind) const {
            return (static_cast<std::size_t>(ind.lhs_mask) << 8) | ind.rhs_index;
        }
    };

private:
    // Input
    config::InputTable input_table_;
    std::vector<std::shared_ptr<SimilarityMetric>> metrics_;

    // Internal state
    std::vector<std::vector<std::string>> column_data_;
    // Per-attribute comparison mode chosen at load time.
    enum class CmpMode { kIds, kNumeric, kGeneric };
    std::vector<CmpMode> cmp_mode_;
    // For exact-equality metrics (or any metric that is 1.0 iff identical, when the
    // threshold is >= 1.0), each column is interned to a dense integer id so the
    // pair-comparison loop compares ids instead of strings.
    std::vector<std::vector<uint32_t>> column_ids_;
    // For numeric metrics, each cell is parsed to a double once at load time.
    // NaN marks an unparseable cell (treated as similarity 0 in the comparison).
    std::vector<std::vector<double>> column_vals_;
    // For string metrics, the per-cell length (avoids recomputing it per pair).
    std::vector<std::vector<size_t>> column_lens_;
    // For exact-equality (kIds) attributes: rows grouped by interned id, so the
    // similarity bitset can be built from in-group pairs in O(sum group^2) instead
    // of O(rows^2). Outer index = attribute, inner index = id.
    std::vector<std::vector<std::vector<size_t>>> equality_groups_;
    uint8_t num_attrs_ = 0;
    std::size_t num_rows_ = 0;
    std::size_t total_pairs_ = 0;

    uint32_t full_mask_;

    // separate bin column on chunk of 64 bit. Backed by huge-page-aware allocator.
    std::vector<std::vector<uint64_t, HugePageAllocator<uint64_t>>> attr_similarity_bits_;
    // Precomputed support per attribute mask (size 2^num_attrs_). O(1) lookup.
    std::vector<std::size_t, HugePageAllocator<std::size_t>> support_index_;

    std::size_t cache_max_size_ = 10000;

    // Parameters
    std::vector<double> min_similarity_;  // similarity thresholds per attribute
    double eps_ = 1.0;                    // minimum confidence for RFD
    std::size_t max_generations_ = 32;
    std::size_t population_size_ = 1024;
    double crossover_probability_ = 1.0;
    double mutation_probability_ = 1.0;
    std::uint32_t seed_ = 123;  // random number generator seed
    RngEngine rng_engine_ = RngEngine::kMt19937;
    config::ThreadNumType threads_ = 0;  // 0 => single-threaded

    std::unordered_set<RFD, RFDHash> discovered_;

    // Algorithm overrides
    void RegisterOptions();
    void MakeExecuteOptsAvailable() final;
    void LoadDataInternal() final;
    void ExecuteInternal() final;
    void ResetState() final;

    using Population = std::vector<Individual>;

    // helper methods
    void BuildSimilarityBitsets();
    // Fills attr_similarity_bits_[a] by comparing every pair of rows on attribute a.
    void BuildAttributeBitset(size_t a);
    // Fills attr_similarity_bits_[a] for row range [i0, i1) only. The written bit
    // positions form a contiguous sub-range of the global pair index, so distinct
    // (a, [i0,i1)) tasks are disjoint and safe to run in parallel.
    void BuildAttributeBitsetRange(size_t a, size_t i0, size_t i1);
    // Equality fast-path: sets bits only for in-group row pairs (p in [i0,i1), q>p).
    void BuildEqualityBitsetRange(size_t a, size_t i0, size_t i1);
    void BuildSupportIndex();
    // Exact-equality fast path: builds support_index_ directly from the interned
    // column ids by partition refinement, without similarity bitsets at all.
    void BuildSupportIndexDirect();
    [[nodiscard]] std::size_t ComputeSupport(uint32_t attrs_mask) const;
    // Computes conf and supp for a single individual
    [[nodiscard]] Individual Evaluate(Individual const& ind) const;
    // Computes conf and supp for all individuals (optionally in parallel)
    void EvaluatePopulation(Population& pop) const;
    // Checks each individual threshold satisfies conf
    [[nodiscard]] bool AllOf(Population const& pop) const;
    // Computes fitness from conf: 1.0 if confidence >= beta, else confidence / beta.
    [[nodiscard]] double Fitness(double confidence) const noexcept;
    // Removes duplicate (lhs_mask, rhs_index) individuals keeping the first occurrence.
    void Deduplicate(Population& pop) const;

    // GA methods
    [[nodiscard]] Population InitializePopulation(Rng& rng) const;
    [[nodiscard]] Population Select(Population const& pop, Rng& rng) const;
    [[nodiscard]] Population Crossover(Population const& selected, Rng& rng) const;
    [[nodiscard]] Population Mutate(Population const& pop, Rng& rng) const;

    [[nodiscard]] std::unordered_set<RFD, RFDHash> Finalize(Population const& pop) const;

    friend class tests::GaRfdTester;

public:
    GaRfd();
    ~GaRfd() override;

    void SetMetrics(std::vector<std::shared_ptr<SimilarityMetric>> metrics) noexcept {
        metrics_ = std::move(metrics);
    }

    [[nodiscard]] std::vector<RFD> GetRfds() const {
        return {discovered_.begin(), discovered_.end()};
    }
};

}  // namespace algos::rfd
