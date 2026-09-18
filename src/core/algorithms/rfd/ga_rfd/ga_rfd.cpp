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
#include <limits>
#include <numeric>
#include <ranges>
#include <string>
#include <unordered_set>

#include "core/algorithms/rfd/distance_metric.h"
#include "core/config/custom_metric/custom_metrics/type.h"
#include "core/config/descriptions.h"
#include "core/config/exceptions.h"
#include "core/config/names.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/config/thread_number/option.h"
#include "core/model/index.h"
#include "core/model/types/mixed_type.h"
#include "core/util/custom_metric/custom_metric.h"
#include "core/util/logger.h"
#include "core/util/worker_thread_pool.h"

namespace {

template <typename T>
inline bool InRangeInclusive(T value, T min, T max) noexcept {
    return min <= value && value <= max;
}

std::string BitRepresentation(uint32_t mask, int num_bits = 31) {
    return std::bitset<32>(mask).to_string().substr(32 - num_bits);
}

inline int FirstSetBitIndex(uint32_t value) noexcept {
    return value == 0 ? -1 : static_cast<int>(std::countr_zero(value));
}

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

GaRfd::GaRfd() : Algorithm() {
    using namespace config::names;
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void GaRfd::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kRfdMinSimilarity, kRfdMinimumConfidence, kPopulationSize,
                           kRfdMaxGenerations, kRfdCrossoverProbability, kRfdMutationProbability,
                           kSeed, kRngEngine, kMetrics, kCacheMaxSize, kThreads,
                           kPrecomputeSupport});
}

void GaRfd::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto get_num_columns = [this]() { return input_table_->GetNumberOfColumns(); };

    auto check_probability_range = [](double value) {
        if (!InRangeInclusive(value, 0.0, 1.0))
            throw config::ConfigurationError("Probability-like option must be in [0, 1]");
    };
    auto check_population_size = [](std::size_t population_size) {
        if (population_size == 0)
            throw config::ConfigurationError("population_size must be positive");
    };

    RegisterOption(config::kTableOpt(&input_table_));
    // GA-RFD default metric is exact equality.
    RegisterOption(
            Option<config::CustomMetricsType>{
                    &metrics_, kMetrics, kDRfdMetrics,
                    Option<config::CustomMetricsType>::DefaultFunc([get_num_columns]() {
                        return config::CustomMetricsType(get_num_columns(), EqualityMetric());
                    })}
                    .SetNormalizeFunc([](config::CustomMetricsType& metrics) {
                        auto equality = EqualityMetric();
                        for (auto& metric : metrics) {
                            if (metric == nullptr ||
                                dynamic_cast<::util::DefaultCustomMetric const*>(
                                        metric.get()) != nullptr) {
                                metric = equality;
                            }
                        }
                    })
                    .SetValueCheck([get_num_columns](config::CustomMetricsType const& metrics) {
                        if (metrics.size() != get_num_columns()) {
                            throw config::ConfigurationError(
                                    "metrics size must match the number of attributes");
                        }
                    }));
    RegisterOption(
            Option<std::vector<double>>{
                    &min_similarity_, kRfdMinSimilarity, kDRfdMinSimilarity,
                    Option<std::vector<double>>::DefaultFunc([get_num_columns]() {
                        return std::vector<double>(get_num_columns(), 1.0);
                    })}
                    .SetNormalizeFunc([get_num_columns](auto& similarities) {
                        if (similarities.empty()) {
                            similarities.assign(get_num_columns(), 1.0);
                        } else if (similarities.size() == 1) {
                            similarities.assign(get_num_columns(), similarities.front());
                        }
                    })
                    .SetValueCheck([get_num_columns](std::vector<double> const& similarities) {
                        if (!std::ranges::all_of(similarities, [](double value) {
                                return InRangeInclusive(value, 0.0, 1.0);
                            })) {
                            throw config::ConfigurationError(
                                    "min_similarity values must be in [0, 1]");
                        }
                        if (similarities.size() != get_num_columns()) {
                            throw config::ConfigurationError(
                                    "min_similarity size must be 1 or match the number of "
                                    "attributes");
                        }
                    }));
    RegisterOption(Option{&min_confidence_, kRfdMinimumConfidence, kDRfdMinimumConfidence, 1.0}
                           .SetValueCheck(check_probability_range));
    RegisterOption(Option{&population_size_, kPopulationSize, kDPopulationSize,
                          static_cast<std::size_t>(1024)}
                           .SetValueCheck(check_population_size));
    RegisterOption(Option{&max_generations_, kRfdMaxGenerations, kDRfdMaxGenerations,
                          static_cast<std::size_t>(32)});
    RegisterOption(Option{&crossover_probability_, kRfdCrossoverProbability,
                          kDRfdCrossoverProbability, 1.0}
                           .SetValueCheck(check_probability_range));
    RegisterOption(
            Option{&mutation_probability_, kRfdMutationProbability, kDRfdMutationProbability, 1.0}
                    .SetValueCheck(check_probability_range));
    RegisterOption(Option{&seed_, kSeed, kDSeed, static_cast<std::uint32_t>(123)});
    RegisterOption(Option{&rng_engine_, kRngEngine, kDRngEngine, RngEngine::kMt19937});
    RegisterOption(config::kThreadNumberOpt(&threads_));
    RegisterOption(Option{&precompute_mode_, kPrecomputeSupport, kDPrecomputeSupport,
                          PrecomputeMode::kAuto});
    RegisterOption(Option{&cache_max_size_, kCacheMaxSize, kDCacheMaxSize,
                          static_cast<std::size_t>(10000)});
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

    LOG_INFO("Loaded {} rows, {} attributes, {} total pairs", num_rows_, num_attributes_,
             total_pairs_);
}

void GaRfd::PrepareExactEquality() {
    column_ids_.clear();
    equality_groups_.clear();
    column_ids_.resize(num_attributes_);
    equality_groups_.resize(num_attributes_);

    auto const& column_data = typed_relation_->GetColumnData();
    for (std::size_t attribute = 0; attribute < num_attributes_; ++attribute) {
        if (min_similarity_[attribute] < 1.0 || !metrics_[attribute]->IsEquality()) continue;
        auto const& column = column_data[attribute];
        std::unordered_map<std::string, uint32_t> id_map;
        id_map.reserve(num_rows_);
        column_ids_[attribute].resize(num_rows_);
        // Nulls get unique ids so they never match.
        for (std::size_t row = 0; row < num_rows_; ++row) {
            if (column.IsNullOrEmpty(row)) {
                uint32_t const id =
                        static_cast<uint32_t>(equality_groups_[attribute].size());
                column_ids_[attribute][row] = id;
                equality_groups_[attribute].push_back({row});
                continue;
            }
            // Doubles use raw bytes (string form can merge distinct values); NaN never matches.
            std::string key;
            if (column.GetValueTypeId(row) == model::TypeId::kDouble) {
                std::byte const* bytes = column.GetValue(row);
                if (column.GetTypeId() == model::TypeId::kMixed) {
                    bytes = model::MixedType::RetrieveValue(bytes);
                }
                double value;
                std::memcpy(&value, bytes, sizeof(double));
                if (std::isnan(value)) {
                    uint32_t const id =
                            static_cast<uint32_t>(equality_groups_[attribute].size());
                    column_ids_[attribute][row] = id;
                    equality_groups_[attribute].push_back({row});
                    continue;
                }
                if (value == 0.0) value = 0.0;  // canonicalize -0.0
                key.assign(reinterpret_cast<char const*>(&value), sizeof(double));
            } else {
                key = column.GetDataAsString(row);
            }
            auto it = id_map.find(key);
            if (it == id_map.end()) {
                uint32_t const fresh =
                        static_cast<uint32_t>(equality_groups_[attribute].size());
                it = id_map.emplace(key, fresh).first;
                equality_groups_[attribute].push_back({});
            }
            uint32_t const id = it->second;
            column_ids_[attribute][row] = id;
            equality_groups_[attribute][id].push_back(row);
        }
    }
    std::size_t exact = 0;
    for (auto const& ids : column_ids_) exact += !ids.empty();
    LOG_INFO("Exact-equality columns: {}/{}", exact, num_attributes_);
}

void GaRfd::BuildMatchBitsetRange(std::size_t attribute, std::size_t row_begin,
                                  std::size_t row_end) {
    auto const& column_data = typed_relation_->GetColumnData();
    auto const& column = column_data[attribute];
    auto& bits = attribute_match_bits_[attribute];
    // Core uses distances internally; the user-facing threshold is a
    // similarity in [0, 1], so distance threshold is 1 - similarity
    double const max_distance = 1.0 - min_similarity_[attribute];
    auto const& metric = *metrics_[attribute];
    model::Type const& column_type = column.GetType();

    // NULLs and empty values never count as matching
    bool const is_mixed = column.GetTypeId() == model::TypeId::kMixed;
    std::vector<bool> valid(is_mixed ? 0 : num_rows_);
    if (!is_mixed) {
        for (std::size_t row = 0; row < num_rows_; ++row) {
            valid[row] = !column.IsNullOrEmpty(row);
        }
    }

    for (std::size_t first_row = row_begin; first_row < row_end; ++first_row) {
        std::byte const* first_value;
        if (is_mixed) {
            first_value = column.GetValue(first_row);
        } else {
            first_value = valid[first_row] ? column.GetValue(first_row) : nullptr;
        }
        if (first_value == nullptr) continue;
        std::size_t const base = first_row * num_rows_ - first_row * (first_row + 1) / 2;
        // One atomic deposit per 64-pair block.
        for (std::size_t second_row = first_row + 1; second_row < num_rows_;
             second_row += 64) {
            std::size_t const pair0 = base + second_row - first_row - 1;
            std::size_t const block_end = std::min(second_row + 64, num_rows_);
            uint64_t word = 0;
            for (std::size_t k = 0; k < block_end - second_row; ++k) {
                bool match = (is_mixed || valid[second_row + k]) &&
                             metric.Dist(&column_type, first_value,
                                         column.GetValue(second_row + k)) <= max_distance;
                if (match) word |= (uint64_t{1} << k);
            }
            if (word != 0) DepositBlock(bits, pair0, word);
        }
    }
}

void GaRfd::BuildMatchBitsets() {
    if (!support_cache_) {
        support_cache_ = std::make_unique<util::LRUCache<uint32_t, std::size_t>>(cache_max_size_);
    }
    support_index_.clear();
    lazy_support_ = false;

    PrepareExactEquality();

    bool const all_exact = std::ranges::all_of(
            column_ids_, [](auto const& ids) { return !ids.empty(); });
    constexpr std::size_t kMaxDirectRows = std::size_t{1} << 22;
    if (all_exact && num_attributes_ < 24 &&
        num_rows_ <= (kMaxDirectRows >> num_attributes_)) {
        BuildSupportIndexDirect();
        return;
    }
    if (all_exact) {
        lazy_support_ = true;
        LOG_INFO("Using lazy per-mask support compute ({} rows, {} attributes)", num_rows_,
                 num_attributes_);
        return;
    }

    std::size_t const num_words_per_attribute = (total_pairs_ + 63) / 64;
    attribute_match_bits_.assign(num_attributes_,
                                 std::vector<uint64_t>(num_words_per_attribute, 0));

    std::size_t const num_threads =
            threads_ > 1 ? static_cast<std::size_t>(threads_) : std::size_t{1};
    // Row chunks keep all threads busy when attributes are few.
    std::size_t const chunks = std::max<std::size_t>(1, std::min(num_threads, num_rows_));
    std::size_t const chunk_size = (num_rows_ + chunks - 1) / chunks;
    if (num_threads > 1) {
        std::size_t const total_tasks = num_attributes_ * chunks;
        ::util::WorkerThreadPool pool(threads_);
        pool.ExecIndex(
                [this, chunks, chunk_size](model::Index t) {
                    std::size_t const attribute = static_cast<std::size_t>(t) / chunks;
                    std::size_t const chunk = static_cast<std::size_t>(t) % chunks;
                    std::size_t const row_begin = chunk * chunk_size;
                    std::size_t const row_end = std::min(row_begin + chunk_size, num_rows_);
                    if (row_begin < row_end) {
                        BuildMatchBitsetRange(attribute, row_begin, row_end);
                    }
                },
                static_cast<model::Index>(total_tasks));
        LOG_INFO("Match bitsets built for {} attributes on {} threads", num_attributes_,
                 num_threads);
    } else {
        for (std::size_t attribute = 0; attribute < num_attributes_; ++attribute) {
            BuildMatchBitsetRange(attribute, 0, num_rows_);
            LOG_INFO("Finished attribute {} match bitset", attribute);
        }
        LOG_INFO("Match bitsets built for {} attributes", num_attributes_);
    }

    // Precompute support for O(1) lookups during evolution.
    std::size_t const words_per_attr = (total_pairs_ + 63) / 64;
    constexpr std::size_t kMaxPrecomputeOps = 1'000'000'000;
    std::size_t const table_size = num_attributes_ >= 32
                                           ? std::numeric_limits<std::size_t>::max()
                                           : (std::size_t{1} << num_attributes_);
    bool const can_precompute =
            num_attributes_ < 20 && table_size <= (std::size_t{1} << 20) &&
            table_size * num_attributes_ * words_per_attr <= kMaxPrecomputeOps;
    bool want_precompute = false;
    if (precompute_mode_ == PrecomputeMode::kOff) {
        LOG_INFO("Skipping support precompute (precompute_support=off); using on-the-fly compute");
    } else if (precompute_mode_ == PrecomputeMode::kOn) {
        constexpr std::size_t kMaxForcedTable = std::size_t{1} << 26;
        if (table_size > kMaxForcedTable) {
            throw config::ConfigurationError(
                    "precompute_support=on requires the support index to hold at most 2^26 "
                    "masks; use auto or off for wider tables");
        }
        want_precompute = true;
    } else {
        want_precompute = can_precompute;
        if (!can_precompute) {
            LOG_INFO(
                    "Skipping support precompute (attrs={}, pairs={}); using on-the-fly compute",
                    num_attributes_, total_pairs_);
        }
    }
    if (want_precompute) {
        BuildSupportIndex();
    }
}

void GaRfd::BuildSupportIndex() {
    std::size_t const table_size = std::size_t{1} << num_attributes_;
    support_index_.assign(table_size, 0);
    support_index_[0] = total_pairs_;

    std::size_t const vec_size =
            attribute_match_bits_.empty() ? 0 : attribute_match_bits_.front().size();

    auto reduce_mask = [this, vec_size](uint32_t mask) {
        thread_local std::vector<uint64_t> buf;
        if (buf.size() != vec_size) buf.resize(vec_size);

        uint32_t mm = mask;
        int a = FirstSetBitIndex(mm);
        mm &= mm - 1;

        std::memcpy(buf.data(), attribute_match_bits_[a].data(),
                    vec_size * sizeof(uint64_t));

        bool zero = false;
        while (mm) {
            int b = FirstSetBitIndex(mm);
            auto const& other = attribute_match_bits_[b];
            std::size_t running = 0;
            for (std::size_t k = 0; k < vec_size; ++k) {
                buf[k] &= other[k];
                running += std::popcount(buf[k]);
            }
            if (running == 0) {
                zero = true;
                break;
            }
            mm &= mm - 1;
        }

        if (zero) {
            support_index_[mask] = 0;
            return;
        }

        std::size_t support = 0;
        for (std::size_t k = 0; k < vec_size; ++k) support += std::popcount(buf[k]);
        support_index_[mask] = support;
    };

    if (threads_ > 1 && table_size > 1) {
        ::util::WorkerThreadPool pool(threads_);
        pool.ExecIndex(
                [&reduce_mask](model::Index m) { reduce_mask(static_cast<uint32_t>(m + 1)); },
                static_cast<model::Index>(table_size - 1));
    } else {
        for (uint32_t mask = 1; mask < table_size; ++mask) reduce_mask(mask);
    }
}

void GaRfd::BuildSupportIndexDirect() {
    std::size_t const table_size = std::size_t{1} << num_attributes_;
    support_index_.assign(table_size, 0);
    support_index_[0] = total_pairs_;
    LOG_INFO("Using direct support precompute ({} rows, {} attributes)", num_rows_,
             num_attributes_);

    std::size_t const slice = num_rows_ * table_size;
    std::size_t const bnd_slice = (num_rows_ + 1) * table_size;
    std::vector<size_t> rows_flat(slice);
    std::vector<size_t> bounds_flat(bnd_slice);
    std::vector<size_t> num_groups(table_size, 0);
    std::iota(rows_flat.begin(), rows_flat.begin() + num_rows_, size_t{0});
    bounds_flat[0] = 0;
    bounds_flat[1] = num_rows_;
    num_groups[0] = 1;

    auto refine_mask = [this, &rows_flat, &bounds_flat, &num_groups](uint32_t mask) {
        uint32_t const parent = mask & (mask - 1);
        int const a = FirstSetBitIndex(mask);
        auto const& col_ids = column_ids_[static_cast<size_t>(a)];
        std::size_t const rows_off = static_cast<std::size_t>(mask) * num_rows_;
        std::size_t const rows_p_off = static_cast<std::size_t>(parent) * num_rows_;
        std::size_t const bnd_off = static_cast<std::size_t>(mask) * (num_rows_ + 1);
        std::size_t const bnd_p_off = static_cast<std::size_t>(parent) * (num_rows_ + 1);

        thread_local std::vector<uint32_t> count;
        thread_local std::vector<uint32_t> off;
        thread_local std::vector<uint32_t> touched;
        if (count.size() < num_rows_) {
            count.assign(num_rows_, 0);
            off.assign(num_rows_, 0);
        }
        touched.clear();

        std::size_t child_groups = 0;
        std::size_t cursor = 0;
        std::size_t support = 0;
        bounds_flat[bnd_off] = 0;
        for (size_t g = 0; g < num_groups[parent]; ++g) {
            std::size_t const b0 = bounds_flat[bnd_p_off + g];
            std::size_t const b1 = bounds_flat[bnd_p_off + g + 1];
            for (std::size_t i = b0; i < b1; ++i) {
                uint32_t const id = col_ids[rows_flat[rows_p_off + i]];
                if (count[id] == 0) {
                    count[id] = 1;
                    touched.push_back(id);
                } else {
                    ++count[id];
                }
            }
            for (uint32_t id : touched) {
                std::size_t const c = count[id];
                support += c * (c - 1) / 2;
                off[id] = static_cast<uint32_t>(cursor);
                cursor += c;
                bounds_flat[bnd_off + ++child_groups] = cursor;
            }
            for (std::size_t i = b0; i < b1; ++i) {
                uint32_t const id = col_ids[rows_flat[rows_p_off + i]];
                rows_flat[rows_off + off[id]++] = rows_flat[rows_p_off + i];
            }
            for (uint32_t id : touched) count[id] = 0;
            touched.clear();
        }
        num_groups[mask] = child_groups;
        support_index_[mask] = support;
    };

    std::optional<::util::WorkerThreadPool> pool;
    if (threads_ > 1) pool.emplace(threads_);
    for (uint32_t pc = 1; pc <= num_attributes_; ++pc) {
        std::vector<uint32_t> level;
        for (uint32_t mask = 1; mask < table_size; ++mask) {
            if (static_cast<uint32_t>(std::popcount(mask)) == pc) level.push_back(mask);
        }
        if (level.empty()) continue;
        if (pool) {
            pool->ExecIndex(
                    [&refine_mask, &level](model::Index i) {
                        refine_mask(level[static_cast<size_t>(i)]);
                    },
                    static_cast<model::Index>(level.size()));
        } else {
            for (uint32_t mask : level) refine_mask(mask);
        }
    }
}

std::size_t GaRfd::ComputeSupportDirect(uint32_t attributes_mask) const {
    uint32_t mm = attributes_mask;
    uint32_t attrs[32];
    int k = 0;
    while (mm) {
        attrs[k++] = static_cast<uint32_t>(FirstSetBitIndex(mm));
        mm &= mm - 1;
    }

    thread_local std::vector<size_t> idx;
    if (idx.size() != num_rows_) idx.resize(num_rows_);
    std::iota(idx.begin(), idx.end(), size_t{0});
    std::sort(idx.begin(), idx.end(), [this, &attrs, k](size_t i, size_t j) {
        for (int t = 0; t < k; ++t) {
            uint32_t const a = column_ids_[attrs[t]][i];
            uint32_t const b = column_ids_[attrs[t]][j];
            if (a != b) return a < b;
        }
        return false;
    });

    std::size_t support = 0;
    size_t run_start = 0;
    for (size_t i = 1; i < num_rows_; ++i) {
        bool same = true;
        for (int t = 0; t < k; ++t) {
            if (column_ids_[attrs[t]][idx[i]] != column_ids_[attrs[t]][idx[i - 1]]) {
                same = false;
                break;
            }
        }
        if (!same) {
            std::size_t const c = i - run_start;
            support += c * (c - 1) / 2;
            run_start = i;
        }
    }
    std::size_t const c = num_rows_ - run_start;
    support += c * (c - 1) / 2;
    return support;
}

std::size_t GaRfd::ComputeSupportLazy(uint32_t attributes_mask) const {
    if (attributes_mask == 0) [[unlikely]] {
        return total_pairs_;
    }

    if ((attributes_mask & (attributes_mask - 1)) == 0u) [[unlikely]] {
        std::size_t s = 0;
        for (auto const& group :
             equality_groups_[static_cast<size_t>(FirstSetBitIndex(attributes_mask))]) {
            s += group.size() * (group.size() - 1) / 2;
        }
        return s;
    }

    if (auto cached = support_cache_->Get(attributes_mask)) return *cached;

    std::size_t const support = ComputeSupportDirect(attributes_mask);

    support_cache_->Put(attributes_mask, support);
    return support;
}

std::size_t GaRfd::ComputeSupport(uint32_t attributes_mask) const {
    if (!support_index_.empty()) [[likely]] {
        return support_index_[attributes_mask];
    }

    if (lazy_support_) [[unlikely]] {
        return ComputeSupportLazy(attributes_mask);
    }

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

    auto const& first_bits = attribute_match_bits_[first_attribute];

    if (first_bits.empty()) [[unlikely]] {
        support_cache_->Put(attributes_mask, 0);
        return 0;
    }

    if ((attributes_mask & (attributes_mask - 1)) == 0u) {
        std::size_t support = 0;
        for (uint64_t word : first_bits) {
            support += std::popcount(word);
        }
        support_cache_->Put(attributes_mask, support);
        LOG_DEBUG("Support for mask {} = {}", BitRepresentation(attributes_mask, num_attributes_),
                  support);
        return support;
    }

    std::size_t const vector_size = first_bits.size();
    if (compute_buffer_.size() != vector_size) {
        compute_buffer_.resize(vector_size);
    }

    std::memcpy(compute_buffer_.data(), first_bits.data(), vector_size * sizeof(uint64_t));

    remaining_mask &= remaining_mask - 1;
    while (remaining_mask != 0) {
        int attribute = FirstSetBitIndex(remaining_mask);
        auto const& other_bits = attribute_match_bits_[attribute];
        std::size_t running_support = 0;
        for (std::size_t word = 0; word < vector_size; ++word) {
            compute_buffer_[word] &= other_bits[word];
            running_support += std::popcount(compute_buffer_[word]);
        }
        if (running_support == 0) [[unlikely]] {
            support_cache_->Put(attributes_mask, 0);
            return 0;
        }
        remaining_mask &= remaining_mask - 1;
    }

    std::size_t support = 0;
    for (std::size_t word = 0; word < vector_size; ++word) {
        support += std::popcount(compute_buffer_[word]);
    }

    support_cache_->Put(attributes_mask, support);
    LOG_DEBUG("Support for mask {} = {}", BitRepresentation(attributes_mask, num_attributes_),
              support);
    return support;
}

GaRfd::Individual GaRfd::Evaluate(Individual const& individual) const {
    uint32_t const lhs_mask = individual.lhs_mask;
    uint8_t const rhs_index = individual.rhs_index;

    double support_lhs = static_cast<double>(ComputeSupport(lhs_mask)) / total_pairs_;
    if (support_lhs == 0.0) [[unlikely]] {
        return {lhs_mask, rhs_index, 0.0, 0.0};
    }

    uint32_t const both_mask = lhs_mask | (1u << rhs_index);
    double support_both = static_cast<double>(ComputeSupport(both_mask)) / total_pairs_;
    double confidence = support_both / support_lhs;
    return {lhs_mask, rhs_index, confidence, support_lhs};
}

void GaRfd::EvaluatePopulation(std::unordered_set<Individual, IndividualHash>& population) const {
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
    population.reserve(population_size_);

    std::uniform_int_distribution<uint8_t> rhs_dist(0, num_attributes_ - 1);
    std::uniform_int_distribution<uint8_t> lhs_size_dist(1, num_attributes_ - 1);
    std::uniform_int_distribution<uint8_t> shuffle_dist;

    std::vector<uint8_t> all_indices(num_attributes_);
    std::iota(all_indices.begin(), all_indices.end(), 0);

    std::vector<uint8_t> pool(num_attributes_);
    uint8_t const last_index = static_cast<uint8_t>(num_attributes_ - 1);

    std::size_t attempts = 0;
    while (population.size() < population_size_ && attempts++ < population_size_ * 2 + 1) {
        uint8_t const rhs_index = rhs_dist(random_generator);
        uint8_t const lhs_size = lhs_size_dist(random_generator);

        std::memcpy(pool.data(), all_indices.data(), num_attributes_ * sizeof(uint8_t));

        std::swap(pool[rhs_index], pool[last_index]);

        for (uint8_t position = 0; position < lhs_size; ++position) {
            using ParamType = std::uniform_int_distribution<uint8_t>::param_type;
            uint8_t swap_position =
                    shuffle_dist(random_generator, ParamType(position, last_index - 1));
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
                               static_cast<std::size_t>(population_size_ + 200)));

    std::uniform_real_distribution<double> uniform01(0.0, 1.0);
    std::bernoulli_distribution coin(0.5);

    for (auto first = selected.begin(); first != selected.end(); ++first) {
        auto second = first;
        ++second;
        for (; second != selected.end(); ++second) {
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

            if (mask_first != 0 && (mask_first & (1u << rhs_first)) == 0)
                offspring.emplace(mask_first, rhs_first, 0.0, 0.0);
            if (mask_second != 0 && (mask_second & (1u << rhs_second)) == 0)
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
    std::uniform_int_distribution<uint8_t> mutation_kind_dist(0, 2);

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
                int set_bits = std::popcount(mask);
                if (set_bits == 0) break;
                int skip = std::uniform_int_distribution<int>(0, set_bits - 1)(random_generator);
                uint32_t remainder = mask;
                while (skip-- > 0) {
                    remainder &= remainder - 1;
                }
                mask ^= (remainder & (~remainder + 1));
                mutated_flag = true;
                break;
            }
            case 1: {  // Add a random available bit to the lhs (excludes bits already in lhs and
                       // rhs)
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
                break;
            }
        }
        if (mutated_flag && mask != 0 && (mask & (1u << rhs_index)) == 0)
            mutated.emplace(mask, rhs_index, 0.0, 0.0);
        else
            mutated.insert(individual);
    }
    return mutated;
}

std::unordered_set<RFD, RFDHash> GaRfd::Finalize(
        std::unordered_set<Individual, IndividualHash> const& population) const {
    std::unordered_set<RFD, RFDHash> result;
    result.reserve(population.size());

    for (auto const& individual : population) {
        if (individual.confidence < min_confidence_) continue;
        result.emplace(individual.lhs_mask, individual.rhs_index, individual.support,
                       individual.confidence);
    }
    LOG_INFO("Finalized {} unique RFDs", result.size());
    return result;
}

void GaRfd::ExecuteInternal() {
    LOG_INFO("Build match bitsets...");
    BuildMatchBitsets();
    Rng random_generator(rng_engine_, seed_);
    auto population = InitializePopulation(random_generator);
    EvaluatePopulation(population);
    for (std::size_t generation = 0; generation < max_generations_; ++generation) {
        LOG_INFO("Generation {}/{} (pop size: {})", generation + 1, max_generations_,
                 population.size());
        if (population.size() >= population_size_ && AllConfidencesAboveThreshold(population)) {
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
        if (population.size() > population_size_ + 100) {
            std::vector<Individual> sorted(population.begin(), population.end());
            std::sort(sorted.begin(), sorted.end(), [](auto const& left, auto const& right) {
                return left.confidence > right.confidence;
            });
            sorted.resize(population_size_ + 100);
            population =
                    std::unordered_set<Individual, IndividualHash>(sorted.begin(), sorted.end());
        }
    }
    discovered_ = Finalize(population);
}

void GaRfd::ResetState() {
    discovered_.clear();
    support_cache_.reset();
    support_index_.clear();
    column_ids_.clear();
    equality_groups_.clear();
    attribute_match_bits_.clear();
    lazy_support_ = false;
}

GaRfd::~GaRfd() {
    ResetState();
}

}  // namespace algos::rfd
