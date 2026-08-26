#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/rfd/ga_rfd/rng_engine.h"
#include "core/algorithms/rfd/ga_rfd/rng_wrapper.h"
#include "core/algorithms/rfd/rfd.h"
#include "core/algorithms/rfd/similarity_metric.h"
#include "core/algorithms/rfd/ga_rfd/util/huge_page_allocator.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/config/thread_number/type.h"

namespace tests {
class GaRfdTester;
}

namespace algos::rfd {

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
    enum class CmpMode { kIds, kNumeric, kGeneric };
    std::vector<CmpMode> cmp_mode_;
    std::vector<std::vector<uint32_t>> column_ids_;
    std::vector<std::vector<double>> column_vals_;
    std::vector<std::vector<size_t>> column_lens_;
    std::vector<std::vector<std::vector<size_t>>> equality_groups_;
    uint8_t num_attrs_ = 0;
    std::size_t num_rows_ = 0;
    std::size_t total_pairs_ = 0;

    uint32_t full_mask_;

    std::vector<std::vector<uint64_t, HugePageAllocator<uint64_t>>> attr_similarity_bits_;
    std::vector<std::size_t, HugePageAllocator<std::size_t>> support_index_;
    bool lazy_support_ = false;
    mutable std::mutex support_cache_mutex_;
    mutable std::unordered_map<uint32_t, std::size_t> support_cache_;

    std::size_t cache_max_size_ = 10000;

    // Parameters
    std::vector<double> min_similarity_;
    double eps_ = 1.0;
    std::size_t max_generations_ = 32;
    std::size_t population_size_ = 1024;
    double crossover_probability_ = 1.0;
    double mutation_probability_ = 1.0;
    std::uint32_t seed_ = 123;
    RngEngine rng_engine_ = RngEngine::kMt19937;
    config::ThreadNumType threads_ = 0;

    std::unordered_set<RFD, RFDHash> discovered_;

    // Algorithm overrides
    void RegisterOptions();
    void MakeExecuteOptsAvailable() final;
    void LoadDataInternal() final;
    void ExecuteInternal() final;
    void ResetState() final;

    using Population = std::vector<Individual>;

    // helper methods
    void PrepareAttributeComparisonModes();
    void BuildSimilarityBitsets();
    void BuildAttributeBitset(size_t a);
    void BuildAttributeBitsetRange(size_t a, size_t i0, size_t i1);
    void BuildEqualityBitsetRange(size_t a, size_t i0, size_t i1);
    void BuildSupportIndex();
    void BuildSupportIndexDirect();
    [[nodiscard]] std::size_t ComputeSupportDirect(uint32_t attrs_mask) const;
    [[nodiscard]] std::size_t ComputeSupportLazy(uint32_t attrs_mask) const;
    [[nodiscard]] std::size_t ComputeSupport(uint32_t attrs_mask) const;
    [[nodiscard]] Individual Evaluate(Individual const& ind) const;
    void EvaluatePopulation(Population& pop) const;
    [[nodiscard]] bool AllOf(Population const& pop) const;
    [[nodiscard]] double Fitness(double confidence) const noexcept;
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
