#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <random>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/rfd/ga_rfd/util/lru_cache.h"
#include "core/algorithms/rfd/rfd.h"
#include "core/config/custom_metric/custom_metrics/type.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/column_layout_typed_relation_data.h"

namespace algos::rfd {

static constexpr std::size_t kMaxAttributes = 32;

using AttributeMask = std::uint32_t;
using LHS_Mask = std::uint32_t;
using RHS_Index = std::uint8_t;

struct Individual {
    LHS_Mask lhs_mask = 0;
    RHS_Index rhs_index = 0;
    double support = 0.0;
    double confidence = 0.0;

    bool operator==(Individual const& other) const {
        return lhs_mask == other.lhs_mask && rhs_index == other.rhs_index;
    }
};

using Population = std::unordered_set<Individual>;

class GaRfd final : public algos::Algorithm {
private:
    // Input
    config::InputTable input_table_;
    config::CustomMetricsOptionType metrics_;

    // Internal state
    std::unique_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    config::CustomMetricsType typed_metrics_;
    std::size_t num_attributes_ = 0;
    std::size_t num_rows_ = 0;
    std::size_t total_pairs_ = 0;

    AttributeMask full_mask_ = 0;

    // One bitset over all row pairs per attribute
    std::vector<boost::dynamic_bitset<std::uint64_t>> similar_pair_bits_;
    boost::dynamic_bitset<std::uint64_t> compute_buffer_;

    std::size_t cache_max_size_ = 10000;
    std::unique_ptr<util::LRUCache<AttributeMask, std::size_t>> support_cache_;

    // Parameters
    std::vector<double> min_similarity_;  // similarity thresholds per attribute in [0, 1]
    double min_confidence_ = 1.0;         // minimum confidence for RFD
    std::size_t max_generations_ = 32;
    std::size_t max_population_size_ = 1024;
    double crossover_probability_ = 1.0;
    double mutation_probability_ = 1.0;
    bool include_key_rfds_ = true;
    std::uint32_t seed_;  // overwritten by the seed option default

    std::vector<RFD> discovered_;

    // Algorithm overrides
    void RegisterOptions();
    void MakeExecuteOptsAvailable() final;
    void LoadDataInternal() final;
    void ExecuteInternal() final;
    void ResetState() final;

    // helper methods
    static Individual MakeIndividual(LHS_Mask lhs_mask, RHS_Index rhs_index);
    static bool IsKey(Individual const& individual) noexcept;
    void BuildTypedMetrics();
    void BuildMatchBitsets();
    std::size_t ComputeSupport(AttributeMask attributes_mask);
    // Computes conf and supp for a single individual
    Individual Evaluate(Individual const& individual);
    // Computes conf and supp for all individuals, moving vacuous (key-based) ones to `keys`
    void EvaluatePopulation(Population& population, Population& keys);
    // Checks every individual's confidence meets the threshold
    bool AllConfidencesAboveThreshold(Population const& population) const;
    // Computes fitness from confidence: 1.0 if confidence >= min_confidence_, else scaled.
    double Fitness(double confidence) const noexcept;
    // Returns true if `lhs_mask` is a minimal key: zero support and no zero-support subset
    bool IsMinimalKey(LHS_Mask lhs_mask);

    // GA methods
    Population InitializePopulation(std::mt19937& random_generator) const;
    Population Select(Population const& population, std::mt19937& random_generator) const;
    Population Crossover(Population const& selected, std::mt19937& random_generator) const;
    Population Mutate(Population const& population, std::mt19937& random_generator) const;

    std::vector<RFD> Finalize(Population const& population, Population const& keys);

public:
    GaRfd();

    [[nodiscard]] std::vector<RFD> const& GetRfds() const {
        return discovered_;
    }
};

}  // namespace algos::rfd

template <>
struct std::hash<algos::rfd::Individual> {
    std::size_t operator()(algos::rfd::Individual const& individual) const noexcept {
        return (static_cast<std::size_t>(individual.lhs_mask) << 8) | individual.rhs_index;
    }
};
