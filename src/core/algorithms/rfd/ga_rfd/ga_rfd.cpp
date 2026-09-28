#include "ga_rfd.h"

#include <algorithm>
#include <atomic>
#include <bit>
#include <bitset>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <numeric>
#include <optional>
#include <random>
#include <ranges>
#include <string>
#include <unordered_set>

#include "core/algorithms/rfd/distance_metric.h"
#include "core/config/descriptions.h"
#include "core/config/exceptions.h"
#include "core/config/names.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/config/thread_number/option.h"
#include "core/model/index.h"
#include "core/util/custom_metric/custom_metric.h"
#include "core/util/logger.h"
#include "core/util/worker_thread_pool.h"

namespace {

[[nodiscard]] std::size_t PopcountAll(std::vector<uint64_t> const& words) noexcept {
    std::size_t support = 0;
    for (uint64_t word : words) support += std::popcount(word);
    return support;
}

std::string BitRepresentation(uint32_t mask, int num_bits = 31) {
    return std::bitset<32>(mask).to_string().substr(32 - num_bits);
}

inline int FirstSetBitIndex(uint32_t value) noexcept {
    return value == 0 ? -1 : static_cast<int>(std::countr_zero(value));
}

inline bool IsBitSet(uint32_t lhs_mask, std::size_t bit_idx) noexcept {
    return (lhs_mask & (1u << bit_idx)) != 0;
}

// Encapsulates "if using more than one thread, create a thread pool, otherwise pass nullptr to
// indicate single-threaded execution" (same pattern as other algorithms).
struct PoolHolder {
private:
    std::optional<::util::WorkerThreadPool> pool_holder_;
    ::util::WorkerThreadPool* pool_ptr_;

public:
    PoolHolder() : pool_holder_(), pool_ptr_(nullptr) {}

    PoolHolder(config::ThreadNumType threads)
        : pool_holder_(threads), pool_ptr_(&*pool_holder_) {}

    ::util::WorkerThreadPool* GetPtr() noexcept {
        return pool_ptr_;
    }
};

// Merges one 64-pair block; atomic for words shared at chunk boundaries.
inline void DepositBlock(std::vector<uint64_t>& bits, std::size_t pair0, uint64_t word) {
    unsigned const r = static_cast<unsigned>(pair0 & 63);
    std::size_t const w0 = pair0 >> 6;
    if (r == 0) {
        std::atomic_ref<uint64_t>(bits[w0]).fetch_or(word, std::memory_order::relaxed);
    } else {
        std::atomic_ref<uint64_t>(bits[w0]).fetch_or(word << r, std::memory_order::relaxed);
        if (w0 + 1 < bits.size())
            std::atomic_ref<uint64_t>(bits[w0 + 1])
                    .fetch_or(word >> (64 - r), std::memory_order::relaxed);
    }
}

}  // namespace

namespace algos::rfd {

GaRfd::GaRfd() {
    using namespace config::names;
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void GaRfd::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kRfdMinSimilarity, kRfdMinimumConfidence, kPopulationSize,
                          kRfdMaxGenerations, kRfdCrossoverProbability, kRfdMutationProbability,
                          kSeed, kRngEngine, kMetrics, kCacheMaxSize, kThreads});
}

void GaRfd::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto get_num_columns = [this]() { return input_table_->GetNumberOfColumns(); };

    auto check_probability_range = [](std::string option_name) {
        return [option_name = std::move(option_name)](double value) {
            if (!(0.0 <= value && value <= 1.0)) {
                throw config::ConfigurationError("Option \"" + option_name +
                                                 "\" must be in [0, 1], got " +
                                                 std::to_string(value));
            }
        };
    };
    auto check_population_size = [](std::size_t population_size) {
        if (population_size == 0)
            throw config::ConfigurationError("population_size must be positive");
    };
    auto default_metrics = [get_num_columns]() {
        return config::CustomMetricsType(get_num_columns(), EqualityMetric());
    };
    auto normalize_metrics = [](config::CustomMetricsType& metrics) {
        for (auto& metric : metrics) {
            if (metric == nullptr) metric = EqualityMetric();
        }
    };
    auto check_metrics = [get_num_columns](config::CustomMetricsType const& metrics) {
        if (metrics.size() != get_num_columns()) {
            throw config::ConfigurationError("metrics size must match the number of attributes");
        }
    };
    auto default_min_similarity = [get_num_columns]() {
        return std::vector<double>(get_num_columns(), 1.0);
    };
    auto normalize_min_similarity = [get_num_columns](std::vector<double>& similarities) {
        if (similarities.size() == 1) {
            double const value = similarities.front();
            similarities.assign(get_num_columns(), value);
        }
    };
    auto check_min_similarity = [get_num_columns](std::vector<double> const& similarities) {
        if (!std::ranges::all_of(similarities,
                                 [](double value) { return 0.0 <= value && value <= 1.0; })) {
            throw config::ConfigurationError("min_similarity values must be in [0, 1]");
        }
        if (similarities.size() != get_num_columns()) {
            throw config::ConfigurationError(
                    "min_similarity size must be 1 or match the number of attributes");
        }
    };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option<config::CustomMetricsType>{
            &metrics_, kMetrics, kDRfdMetrics,
            Option<config::CustomMetricsType>::DefaultFunc(default_metrics)}
                           .SetNormalizeFunc(normalize_metrics)
                           .SetValueCheck(check_metrics));
    RegisterOption(Option<std::vector<double>>{
            &min_similarity_, kRfdMinSimilarity, kDRfdMinSimilarity,
            Option<std::vector<double>>::DefaultFunc(default_min_similarity)}
                           .SetNormalizeFunc(normalize_min_similarity)
                           .SetValueCheck(check_min_similarity));
    RegisterOption(Option{&min_confidence_, kRfdMinimumConfidence, kDRfdMinimumConfidence, 1.0}
                           .SetValueCheck(check_probability_range(kRfdMinimumConfidence)));
    RegisterOption(Option{&max_population_size_, kPopulationSize, kDPopulationSize,
                          static_cast<std::size_t>(1024)}
                           .SetValueCheck(check_population_size));
    RegisterOption(Option{&max_generations_, kRfdMaxGenerations, kDRfdMaxGenerations,
                          static_cast<std::size_t>(32)});
    RegisterOption(Option{&crossover_probability_, kRfdCrossoverProbability,
                          kDRfdCrossoverProbability, 1.0}
                           .SetValueCheck(check_probability_range(kRfdCrossoverProbability)));
    RegisterOption(
            Option{&mutation_probability_, kRfdMutationProbability, kDRfdMutationProbability, 1.0}
                    .SetValueCheck(check_probability_range(kRfdMutationProbability)));
    RegisterOption(
            Option{&seed_, kSeed, kDSeed, static_cast<std::uint32_t>(std::random_device{}())});
    RegisterOption(Option{&rng_engine_, kRngEngine, kDRngEngine, RngEngine::kMt19937});
    RegisterOption(Option{&cache_max_size_, kCacheMaxSize, kDCacheMaxSize,
                          static_cast<std::size_t>(10000)});
    RegisterOption(config::kThreadNumberOpt(&threads_));
}

void GaRfd::LoadDataInternal() {
    typed_relation_ = model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_, true);
    input_table_->Reset();

    num_attributes_ = typed_relation_->GetNumColumns();
    num_rows_ = typed_relation_->GetNumRows();
    if (num_attributes_ < 2)
        throw config::ConfigurationError("GA-RFD requires at least 2 attributes");
    if (num_attributes_ > kMaxAttributes)
        throw config::ConfigurationError("GA-RFD supports at most 31 attributes");
    if (num_rows_ < 2) throw config::ConfigurationError("GA-RFD requires at least 2 rows");
    full_mask_ = (1u << num_attributes_) - 1;

    total_pairs_ = num_rows_ * (num_rows_ - 1) / 2;

    column_names_.resize(num_attributes_);
    for (std::size_t i = 0; i < num_attributes_; ++i) {
        column_names_[i] = input_table_->GetColumnName(i);
    }

    for (auto& metric : metrics_) {
        if (metric == nullptr) metric = EqualityMetric();
    }
}

void GaRfd::BuildAbsDiffBitsetRange(std::size_t attribute, std::size_t row_begin,
                                             std::size_t row_end,
                                             model::INumericType const* numeric,
                                             std::vector<bool> const& valid,
                                             double max_distance) {
    auto const& column = typed_relation_->GetColumnData()[attribute];
    auto& bits = similar_pair_bits_[attribute];
    // One virtual decode per value; the pair loop below is pure arithmetic.
    std::vector<double> values(num_rows_, 0.0);
    for (std::size_t row = 0; row < num_rows_; ++row) {
        if (valid[row]) values[row] = numeric->GetValueAs<double>(column.GetValue(row));
    }
    for (std::size_t first_row = row_begin; first_row < row_end; ++first_row) {
        if (!valid[first_row]) continue;
        double const first_value = values[first_row];
        double const abs_first = std::abs(first_value);
        std::size_t const base = first_row * num_rows_ - first_row * (first_row + 1) / 2;
        for (std::size_t second_row = first_row + 1; second_row < num_rows_;
             second_row += 64) {
            std::size_t const pair0 = base + second_row - first_row - 1;
            std::size_t const block_end = std::min(second_row + 64, num_rows_);
            uint64_t word = 0;
            for (std::size_t k = 0; k < block_end - second_row; ++k) {
                if (!valid[second_row + k]) continue;
                double const second_value = values[second_row + k];
                double const max_absolute = std::max(abs_first, std::abs(second_value));
                double const distance = (max_absolute == 0.0)
                                                ? 0.0
                                                : std::abs(first_value - second_value) /
                                                          max_absolute;
                if (distance <= max_distance) word |= (uint64_t{1} << k);
            }
            if (word != 0) DepositBlock(bits, pair0, word);
        }
    }
}

void GaRfd::BuildMatchBitsetRange(std::size_t attribute, std::size_t row_begin,
                                           std::size_t row_end,
                                           std::vector<bool> const& valid) {
    auto const& column = typed_relation_->GetColumnData()[attribute];
    // Core uses distances internally; the user-facing threshold is a
    // similarity in [0, 1], so distance threshold is 1 - similarity
    double const max_distance = 1.0 - min_similarity_[attribute];
    auto const& metric = *metrics_[attribute];
    model::Type const& column_type = column.GetType();

    // Decoded fast path for absolute difference on plain numeric columns.
    model::TypeId const type_id = column_type.GetTypeId();
    if (metric.IsAbsoluteDifference() &&
        (type_id == model::TypeId::kInt || type_id == model::TypeId::kDouble)) {
        auto const* numeric = static_cast<model::INumericType const*>(&column_type);
        BuildAbsDiffBitsetRange(attribute, row_begin, row_end, numeric, valid, max_distance);
        return;
    }

    auto& bits = similar_pair_bits_[attribute];

    for (std::size_t first_row = row_begin; first_row < row_end; ++first_row) {
        if (!valid[first_row]) continue;
        std::byte const* first_value = column.GetValue(first_row);
        // Triangular pair number of (first_row, first_row + 1).
        std::size_t const base =
                first_row * num_rows_ - first_row * (first_row + 1) / 2;
        for (std::size_t second_row = first_row + 1; second_row < num_rows_;
             second_row += 64) {
            std::size_t const pair0 = base + second_row - first_row - 1;
            std::size_t const block_end = std::min(second_row + 64, num_rows_);
            uint64_t word = 0;
            for (std::size_t k = 0; k < block_end - second_row; ++k) {
                // NULLs and empty values never count as matching
                bool const match =
                        valid[second_row + k] &&
                        metric.Dist(&column_type, first_value,
                                    column.GetValue(second_row + k)) <= max_distance;
                if (match) word |= (uint64_t{1} << k);
            }
            if (word != 0) DepositBlock(bits, pair0, word);
        }
    }
}

void GaRfd::BuildMatchBitsets() {
    support_cache_ = std::make_unique<util::LRUCache<uint32_t, std::size_t>>(cache_max_size_);

    std::size_t const num_words_per_attribute = (total_pairs_ + 63) / 64;
    similar_pair_bits_.clear();
    similar_pair_bits_.reserve(num_attributes_);
    for (std::size_t attribute = 0; attribute < num_attributes_; ++attribute) {
        similar_pair_bits_.emplace_back(num_words_per_attribute, 0);
    }

    auto const& column_data = typed_relation_->GetColumnData();
    if (pool_ == nullptr) {
        // NULLs and empty values never count as matching
        std::vector<bool> valid(num_rows_);
        for (std::size_t attribute = 0; attribute < num_attributes_; ++attribute) {
            auto const& column = column_data[attribute];
            for (std::size_t row = 0; row < num_rows_; ++row) {
                valid[row] = !column.IsNullOrEmpty(row);
            }
            BuildMatchBitsetRange(attribute, 0, num_rows_, valid);
            LOG_INFO("Finished attribute {} match bitset", attribute);
        }
        LOG_INFO("Match bitsets built for {} attributes", num_attributes_);
        return;
    }
    std::size_t const num_threads = threads_;
    // Row chunks keep all threads busy when attributes are few.
    std::size_t const chunks = std::max<std::size_t>(1, std::min(num_threads, num_rows_));
    std::size_t const chunk_size = (num_rows_ + chunks - 1) / chunks;
    std::size_t const total_tasks = num_attributes_ * chunks;
    assert(pool_ != nullptr);
    auto& pool = *pool_;
    pool.ExecIndex(
            [this, chunks, chunk_size](model::Index t) {
                std::size_t const attribute = static_cast<std::size_t>(t) / chunks;
                std::size_t const chunk = static_cast<std::size_t>(t) % chunks;
                std::size_t const row_begin = chunk * chunk_size;
                std::size_t const row_end = std::min(row_begin + chunk_size, num_rows_);
                auto const& column = typed_relation_->GetColumnData()[attribute];
                // NULLs and empty values never count as matching
                std::vector<bool> local_valid(num_rows_);
                for (std::size_t row = 0; row < num_rows_; ++row) {
                    local_valid[row] = !column.IsNullOrEmpty(row);
                }
                BuildMatchBitsetRange(attribute, row_begin, row_end, local_valid);
            },
            static_cast<model::Index>(total_tasks));
    LOG_INFO("Match bitsets built for {} attributes", num_attributes_);
}

bool GaRfd::AllMetricsThreadSafe() const {
    return std::ranges::all_of(metrics_, [](auto const& metric) {
        return metric == nullptr || metric->IsThreadSafe();
    });
}

std::size_t GaRfd::ComputeSupport(uint32_t attributes_mask) {
    if (auto cached = support_cache_->Get(attributes_mask)) return *cached;

    if (attributes_mask == 0) [[unlikely]] {
        support_cache_->Put(0, total_pairs_);
        return total_pairs_;
    }

    uint32_t remaining_mask = attributes_mask;
    int first_attribute = FirstSetBitIndex(remaining_mask);
    if (first_attribute < 0) [[unlikely]] {
        support_cache_->Put(attributes_mask, 0);
        return 0;
    }

    auto const& first_bits = similar_pair_bits_[first_attribute];

    if (first_bits.empty()) [[unlikely]] {
        support_cache_->Put(attributes_mask, 0);
        return 0;
    }

    if ((attributes_mask & (attributes_mask - 1)) == 0u) {
        std::size_t const support = PopcountAll(first_bits);
        support_cache_->Put(attributes_mask, support);
        LOG_DEBUG("Support for mask {} = {}", BitRepresentation(attributes_mask, num_attributes_),
                  support);
        return support;
    }

    if (compute_buffer_.size() != first_bits.size()) compute_buffer_.resize(first_bits.size());

    remaining_mask &= remaining_mask - 1;
    // Parallel AND-reduce when the vector is big enough to amortize dispatch.
    constexpr std::size_t kMinParallelWords = 65536;
    bool const parallel_reduce =
            pool_ != nullptr && compute_buffer_.size() >= kMinParallelWords;
    std::size_t const num_jobs = parallel_reduce ? static_cast<std::size_t>(threads_) : 1;
    std::vector<std::size_t> partial(num_jobs, 0);
    std::size_t support = 0;
    bool first_step = true;
    while (remaining_mask != 0) {
        int attribute = FirstSetBitIndex(remaining_mask);
        auto const& other_bits = similar_pair_bits_[attribute];
        std::size_t running_support = 0;
        if (parallel_reduce) {
            std::size_t const chunk = (compute_buffer_.size() + num_jobs - 1) / num_jobs;
            bool const copy = first_step;
            pool_->ExecIndex(
                    [this, &first_bits, &other_bits, &partial, chunk, copy](model::Index j) {
                        std::size_t const begin = static_cast<std::size_t>(j) * chunk;
                        std::size_t const end =
                                std::min(begin + chunk, compute_buffer_.size());
                        std::size_t local = 0;
                        if (copy) {
                            for (std::size_t word = begin; word < end; ++word) {
                                uint64_t const v = first_bits[word] & other_bits[word];
                                compute_buffer_[word] = v;
                                local += std::popcount(v);
                            }
                        } else {
                            for (std::size_t word = begin; word < end; ++word) {
                                compute_buffer_[word] &= other_bits[word];
                                local += std::popcount(compute_buffer_[word]);
                            }
                        }
                        partial[static_cast<std::size_t>(j)] = local;
                    },
                    static_cast<model::Index>(num_jobs));
            for (std::size_t s : partial) running_support += s;
        } else if (first_step) {
            for (std::size_t word = 0; word < compute_buffer_.size(); ++word) {
                uint64_t const v = first_bits[word] & other_bits[word];
                compute_buffer_[word] = v;
                running_support += std::popcount(v);
            }
        } else {
            for (std::size_t word = 0; word < compute_buffer_.size(); ++word) {
                compute_buffer_[word] &= other_bits[word];
                running_support += std::popcount(compute_buffer_[word]);
            }
        }
        first_step = false;
        if (running_support == 0) [[unlikely]] {
            support_cache_->Put(attributes_mask, 0);
            return 0;
        }
        support = running_support;
        remaining_mask &= remaining_mask - 1;
    }

    support_cache_->Put(attributes_mask, support);
    LOG_DEBUG("Support for mask {} = {}", BitRepresentation(attributes_mask, num_attributes_),
              support);
    return support;
}

GaRfd::Individual GaRfd::Evaluate(Individual const& individual) {
    uint32_t const lhs_mask = individual.lhs_mask;
    uint8_t const rhs_index = individual.rhs_index;

    double support_lhs = static_cast<double>(ComputeSupport(lhs_mask)) / total_pairs_;
    if (support_lhs == 0.0) [[unlikely]] {
        return {lhs_mask, rhs_index, 0.0, 0.0};
    }

    uint32_t const both_mask = lhs_mask | (1u << rhs_index);
    double support_both = static_cast<double>(ComputeSupport(both_mask)) / total_pairs_;
    double confidence = support_both / support_lhs;
    return {lhs_mask, rhs_index, support_lhs, confidence};
}

void GaRfd::EvaluatePopulation(std::unordered_set<Individual, IndividualHash>& population) {
    auto it = population.begin();
    while (it != population.end()) {
        auto node = population.extract(it++);
        if (!node.empty()) {
            node.value() = Evaluate(node.value());
            population.insert(std::move(node));
        }
    }
}

bool GaRfd::AllConfidencesAboveThreshold(
        std::unordered_set<Individual, IndividualHash> const& population) const {
    assert(!population.empty());
    return std::ranges::all_of(population, [this](Individual const& individual) {
        return individual.confidence >= min_confidence_;
    });
}

double GaRfd::Fitness(double confidence) const noexcept {
    return confidence >= min_confidence_ ? 1.0 : confidence / min_confidence_;
}

std::unordered_set<GaRfd::Individual, GaRfd::IndividualHash> GaRfd::InitializePopulation(
        Rng& random_generator) const {
    std::unordered_set<Individual, IndividualHash> population;
    population.reserve(max_population_size_);

    std::uniform_int_distribution<unsigned> rhs_dist(0, num_attributes_ - 1);
    std::uniform_int_distribution<unsigned> lhs_size_dist(1, num_attributes_ - 1);
    std::uniform_int_distribution<unsigned> shuffle_dist;

    std::vector<uint8_t> all_indices(num_attributes_);
    std::iota(all_indices.begin(), all_indices.end(), 0);

    std::vector<uint8_t> pool(num_attributes_);
    uint8_t const last_index = static_cast<uint8_t>(num_attributes_ - 1);

    std::size_t attempts = 0;
    while (population.size() < max_population_size_ && attempts++ < max_population_size_ * 2 + 1) {
        uint8_t const rhs_index = static_cast<uint8_t>(rhs_dist(random_generator));
        uint8_t const lhs_size = static_cast<uint8_t>(lhs_size_dist(random_generator));

        std::memcpy(pool.data(), all_indices.data(), num_attributes_ * sizeof(uint8_t));

        std::swap(pool[rhs_index], pool[last_index]);

        for (uint8_t position = 0; position < lhs_size; ++position) {
            using ParamType = std::uniform_int_distribution<unsigned>::param_type;
            uint8_t swap_position = static_cast<uint8_t>(
                    shuffle_dist(random_generator, ParamType(position, last_index - 1)));
            std::swap(pool[position], pool[swap_position]);
        }

        uint32_t lhs_mask = 0;
        for (uint8_t position = 0; position < lhs_size; ++position) {
            lhs_mask |= (1u << pool[position]);
        }

        population.insert(Individual{lhs_mask, rhs_index, 0.0, 0.0});
    }
    return population;
}

std::unordered_set<GaRfd::Individual, GaRfd::IndividualHash> GaRfd::Select(
        std::unordered_set<Individual, IndividualHash> const& population,
        Rng& random_generator) const {
    std::unordered_set<Individual, IndividualHash> selected;
    selected.reserve(population.size());

    std::uniform_real_distribution<double> uniform01(0.0, 1.0);

    Individual const* best_individual = nullptr;
    double best_confidence = -1.0;

    for (auto const& individual : population) {
        if (individual.confidence > best_confidence) {
            best_confidence = individual.confidence;
            best_individual = &individual;
        }

        if (uniform01(random_generator) < Fitness(individual.confidence)) {
            selected.emplace(individual);
        }
    }

    if (selected.empty()) {
        selected.emplace(*best_individual);
    }

    return selected;
}

std::unordered_set<GaRfd::Individual, GaRfd::IndividualHash> GaRfd::Crossover(
        std::unordered_set<Individual, IndividualHash> const& selected,
        Rng& random_generator) const {
    std::unordered_set<Individual, IndividualHash> offspring;
    std::size_t const selected_size = selected.size();
    if (selected_size < 2) return offspring;

    offspring.reserve(std::min(selected_size * (selected_size - 1),
                               static_cast<std::size_t>(max_population_size_ + 200)));

    std::uniform_real_distribution<double> uniform01(0.0, 1.0);
    std::bernoulli_distribution coin(0.5);

    for (auto first = selected.begin(); first != selected.end(); ++first) {
        for (auto second = std::next(first); second != selected.end(); ++second) {
            if (uniform01(random_generator) >= crossover_probability_) continue;

            Individual const& parent_first = *first;
            Individual const& parent_second = *second;

            uint32_t mask_first = parent_first.lhs_mask;
            uint32_t mask_second = parent_second.lhs_mask;
            uint8_t rhs_first = parent_first.rhs_index;
            uint8_t rhs_second = parent_second.rhs_index;

            uint32_t differing_bits = mask_first ^ mask_second;
            if (differing_bits != 0) {
                int differing_count = std::popcount(differing_bits);
                int swaps_count = differing_count > 0
                                          ? std::uniform_int_distribution<int>(
                                                    1, differing_count)(random_generator)
                                          : 0;
                while (swaps_count-- > 0) {
                    uint32_t bit = differing_bits & (~differing_bits + 1);
                    mask_first ^= bit;
                    mask_second ^= bit;
                    differing_bits &= differing_bits - 1;
                }
            }

            if (rhs_first != rhs_second && coin(random_generator)) std::swap(rhs_first, rhs_second);

            if (mask_first != 0 && !IsBitSet(mask_first, rhs_first))
                offspring.emplace(mask_first, rhs_first, 0.0, 0.0);
            if (mask_second != 0 && !IsBitSet(mask_second, rhs_second))
                offspring.emplace(mask_second, rhs_second, 0.0, 0.0);
        }
    }
    return offspring;
}

std::unordered_set<GaRfd::Individual, GaRfd::IndividualHash> GaRfd::Mutate(
        std::unordered_set<Individual, IndividualHash> const& population,
        Rng& random_generator) const {
    std::unordered_set<Individual, IndividualHash> mutated;
    mutated.reserve(population.size());

    std::uniform_real_distribution<double> uniform01(0.0, 1.0);
    std::uniform_int_distribution<unsigned> mutation_kind_dist(0, 2);

    for (auto const& individual : population) {
        if (uniform01(random_generator) >= mutation_probability_) {
            mutated.insert(individual);
            continue;
        }

        uint32_t mask = individual.lhs_mask;
        uint8_t rhs_index = individual.rhs_index;

        bool mutated_flag = false;
        switch (mutation_kind_dist(random_generator)) {
            case 0: {  // Remove one random set bit from the mask
                if (mask == 0) break;
                std::uniform_int_distribution<std::size_t> bit_dist(0, num_attributes_ - 1);
                while (true) {
                    std::size_t const bit_to_change = bit_dist(random_generator);
                    if (IsBitSet(mask, bit_to_change)) {
                        mask &= ~(1u << bit_to_change);
                        break;
                    }
                }
                mutated_flag = true;
                break;
            }
            case 1: {  // Add (if possible) a random available bit to the lhs
                       // (excludes bits already in lhs and rhs)
                uint32_t available = full_mask_ & ~mask & ~(1u << rhs_index);
                if (available == 0) break;
                int available_count = std::popcount(available);
                int skip = std::uniform_int_distribution<int>(
                        0, available_count - 1)(random_generator);
                uint32_t remainder = available;
                while (skip-- > 0) {
                    remainder &= remainder - 1;
                }
                mask |= (remainder & (~remainder + 1));
                mutated_flag = true;
                break;
            }
            case 2: {  // Move the rhs variable to a new available bit (not in lhs and curr rhs)
                uint32_t available = full_mask_ & ~mask & ~(1u << rhs_index);
                if (available == 0) break;
                int available_count = std::popcount(available);
                int skip = std::uniform_int_distribution<int>(
                        0, available_count - 1)(random_generator);
                uint32_t remainder = available;
                while (skip-- > 0) {
                    remainder &= remainder - 1;
                }
                rhs_index = static_cast<uint8_t>(std::countr_zero(remainder & (~remainder + 1)));
                mutated_flag = true;
                break;
            }
            default: {
                assert(false);
                __builtin_unreachable();
            }
        }
        if (mutated_flag && mask != 0 && !IsBitSet(mask, rhs_index))
            mutated.emplace(mask, rhs_index, 0.0, 0.0);
        else
            mutated.insert(individual);
    }
    return mutated;
}

std::vector<RFD> GaRfd::Finalize(
        std::unordered_set<Individual, IndividualHash> const& population) const {
    std::vector<RFD> result;
    result.reserve(population.size());

    for (auto const& individual : population) {
        if (individual.confidence < min_confidence_) continue;
        RFD rfd;
        rfd.support = individual.support;
        rfd.confidence = individual.confidence;
        rfd.rhs_index = individual.rhs_index;
        rfd.rhs = column_names_[individual.rhs_index];
        for (std::size_t attribute = 0; attribute < num_attributes_; ++attribute) {
            if (!IsBitSet(individual.lhs_mask, attribute)) continue;
            rfd.lhs_indices.push_back(attribute);
            rfd.lhs.push_back(column_names_[attribute]);
        }
        result.push_back(std::move(rfd));
    }
    LOG_INFO("Finalized {} unique RFDs", result.size());
    return result;
}

void GaRfd::ExecuteInternal() {
    LOG_INFO("Build match bitsets...");
    // Pool lives for the whole execution (build and parallel support reduces).
    // Metrics that are not thread-safe (e.g. Python-backed) force the
    // single-threaded build: worker threads never hold the Python GIL.
    PoolHolder pool_holder =
            (threads_ > 1 && AllMetricsThreadSafe()) ? PoolHolder{threads_} : PoolHolder{};
    pool_ = pool_holder.GetPtr();
    BuildMatchBitsets();
    Rng random_generator(rng_engine_, seed_);
    auto population = InitializePopulation(random_generator);
    EvaluatePopulation(population);
    for (std::size_t generation = 0; generation < max_generations_; ++generation) {
        LOG_INFO("Generation {}/{} (pop size: {})", generation + 1, max_generations_,
                 population.size());
        if (population.size() >= max_population_size_ && AllConfidencesAboveThreshold(population)) {
            LOG_INFO("All individuals satisfy confidence threshold - stopping early");
            break;
        }
        if (population.empty()) [[unlikely]] {
            LOG_INFO("Population is empty, stopping evolution");
            break;
        }
        auto selected = Select(population, random_generator);
        auto offspring = Crossover(selected, random_generator);
        auto mutated = Mutate(selected, random_generator);
        population = std::move(selected);
        population.insert(offspring.begin(), offspring.end());
        population.insert(mutated.begin(), mutated.end());
        EvaluatePopulation(population);
        if (population.size() > max_population_size_ + 100) {
            std::vector<Individual> sorted(population.begin(), population.end());
            std::sort(sorted.begin(), sorted.end(), [](auto const& left, auto const& right) {
                return left.confidence > right.confidence;
            });
            sorted.resize(max_population_size_ + 100);
            population =
                    std::unordered_set<Individual, IndividualHash>(sorted.begin(), sorted.end());
        }
    }
    discovered_ = Finalize(population);
    pool_ = nullptr;
}

void GaRfd::ResetState() {
    discovered_.clear();
    support_cache_.reset();
}

}  // namespace algos::rfd
