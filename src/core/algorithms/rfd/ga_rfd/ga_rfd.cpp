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
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>

#ifdef __linux__
#include <sys/mman.h>
#endif

#include "core/config/descriptions.h"
#include "core/config/names.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/config/thread_number/option.h"
#include "core/model/index.h"
#include "core/util/logger.h"
#include "core/util/worker_thread_pool.h"

namespace {

template <typename T>
[[nodiscard]] inline bool InRangeInclusive(T value, T min, T max) noexcept {
    return min <= value && value <= max;
}

// Inner similarity-bitset vector, backed by the huge-page-aware allocator.
using AttrBits = std::vector<uint64_t, algos::rfd::HugePageAllocator<uint64_t>>;

[[nodiscard]] std::string BitRepresentation(uint32_t mask, int num_bits = 31) {
    return std::bitset<32>(mask).to_string().substr(32 - num_bits);
}

[[nodiscard]] inline int FirstSetBit(uint32_t x) noexcept {
    return x ? __builtin_ctz(x) : -1;
}

}  // namespace

namespace algos::rfd {

std::string RFD::ToString() const {
    std::string res = "[";
    bool first = true;
    for (uint8_t i = 0; i < kMaxAttributes; i++) {
        if (lhs_mask & (1u << i)) {
            if (!first) res += ", ";
            res += std::to_string(i);
            first = false;
        }
    }
    res += "] -> " + std::to_string(rhs_index) + " (conf=" + std::to_string(confidence) +
           ", supp=" + std::to_string(support) + ")";
    return res;
}

GaRfd::GaRfd() : Algorithm() {
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

    auto check_prob_range = [](double v) { return InRangeInclusive(v, 0.0, 1.0); };
    auto check_sim = [check_prob_range](auto const& vec) {
        return std::ranges::all_of(vec, check_prob_range);
    };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option<std::vector<std::shared_ptr<SimilarityMetric>>>{
            &metrics_, kMetrics, kDRfdMetrics, [this]() { return metrics_; }});
    RegisterOption(Option{&min_similarity_, kRfdMinSimilarity, kDRfdMinSimilarity,
                          std::vector<double>{1.0}}
                           .SetValueCheck(check_sim));
    RegisterOption(Option{&eps_, kRfdMinimumConfidence, kDRfdMinimumConfidence, 1.0}.SetValueCheck(
            check_prob_range));
    RegisterOption(Option{&population_size_, kPopulationSize, kDPopulationSize,
                          static_cast<std::size_t>(1024)}
                           .SetValueCheck([](auto v) { return v > 0; }));
    RegisterOption(Option{&max_generations_, kRfdMaxGenerations, kDRfdMaxGenerations,
                          static_cast<std::size_t>(32)});
    RegisterOption(Option{&crossover_probability_, kRfdCrossoverProbability,
                          kDRfdCrossoverProbability, 1.0}
                           .SetValueCheck(check_prob_range));
    RegisterOption(
            Option{&mutation_probability_, kRfdMutationProbability, kDRfdMutationProbability, 1.0}
                    .SetValueCheck(check_prob_range));
    RegisterOption(Option{&seed_, kSeed, kDSeed, static_cast<std::uint32_t>(123)});
    RegisterOption(Option{&rng_engine_, kRngEngine, kDRngEngine, RngEngine::kMt19937});
    RegisterOption(config::kThreadNumberOpt(&threads_));
    RegisterOption(
            Option{&cache_max_size_, kCacheMaxSize, kDCacheMaxSize, static_cast<std::size_t>(10000)}
                    .SetValueCheck([](auto v) { return v >= 0; }));
}

void GaRfd::LoadDataInternal() {
    num_rows_ = 0;
    column_data_.clear();

    if (!input_table_->HasNextRow()) throw std::runtime_error("Input table is empty");

    auto first_row = input_table_->GetNextRow();
    num_attrs_ = first_row.size();
    if (num_attrs_ < 2) throw std::runtime_error("GA-rfd requires at least 2 attributes");
    if (num_attrs_ > kMaxAttributes) throw std::runtime_error("Maximum 31 attributes supported");
    full_mask_ = (1u << num_attrs_) - 1;

    constexpr size_t k_reserve_chunk = 1024;
    column_data_.resize(num_attrs_);
    for (auto& col : column_data_) col.reserve(k_reserve_chunk);

    for (size_t i = 0; i < num_attrs_; ++i) column_data_[i].emplace_back(std::move(first_row[i]));
    num_rows_ = 1;

    while (input_table_->HasNextRow()) {
        auto row = input_table_->GetNextRow();
        if (row.size() != num_attrs_)
            throw std::runtime_error("Inconsistent number of attributes in row");
        for (size_t i = 0; i < num_attrs_; ++i) column_data_[i].emplace_back(std::move(row[i]));
        ++num_rows_;
    }

    if (num_rows_ < 2) throw std::runtime_error("Input table must contain at least 2 rows");

    if (num_rows_ > std::numeric_limits<std::size_t>::max() / (num_rows_ - 1) / 2)
        throw std::runtime_error("Table too large, total pairs would overflow size_t");
    total_pairs_ = num_rows_ * (num_rows_ - 1) / 2;

    LOG_INFO("Loaded {} rows, {} attributes, {} total pairs", num_rows_, num_attrs_, total_pairs_);

    if (min_similarity_.empty()) {
        min_similarity_.assign(num_attrs_, 1.0);
    } else if (min_similarity_.size() == 1) {
        min_similarity_.assign(num_attrs_, min_similarity_[0]);
    } else if (min_similarity_.size() != num_attrs_) {
        throw std::invalid_argument("min_similarity size must match the number of attributes");
    }

    if (metrics_.empty()) {
        metrics_.clear();
        metrics_.reserve(num_attrs_);
        for (size_t i = 0; i < num_attrs_; i++) metrics_.emplace_back(EqualityMetric());
    }
    if (metrics_.size() != num_attrs_)
        throw std::invalid_argument("The number of attributes and metrics do not match");

    // Pick a comparison mode per attribute and precompute whatever that mode needs,
    // so the pair-comparison loop avoids strings / parsing / virtual dispatch:
    //   kIds     - exact-similarity metric with threshold >= 1.0: intern to int ids.
    //   kNumeric  - numeric metric: parse each cell to a double once.
    //   kGeneric  - anything else: compare strings via metric.Satisfies.
    cmp_mode_.resize(num_attrs_);
    column_ids_.clear();
    column_vals_.clear();
    column_lens_.clear();
    equality_groups_.clear();
    column_ids_.resize(num_attrs_);
    column_vals_.resize(num_attrs_);
    column_lens_.resize(num_attrs_);
    equality_groups_.resize(num_attrs_);

    for (size_t a = 0; a < num_attrs_; ++a) {
        auto const& col = column_data_[a];
        double const min_sim = min_similarity_[a];

        if (metrics_[a]->IsExactSimilarity() && min_sim >= 1.0) {
            cmp_mode_[a] = CmpMode::kIds;
            std::unordered_map<std::string, uint32_t> id_map;
            id_map.reserve(num_rows_);
            column_ids_[a].resize(num_rows_);
            for (size_t i = 0; i < num_rows_; ++i) {
                auto it = id_map.find(col[i]);
                if (it == id_map.end()) {
                    it = id_map.emplace(col[i], static_cast<uint32_t>(id_map.size())).first;
                }
                column_ids_[a][i] = it->second;
            }
            // Group row indices by id for the O(sum group^2) equality bitset build.
            auto& groups = equality_groups_[a];
            groups.resize(id_map.size());
            for (size_t i = 0; i < num_rows_; ++i) groups[column_ids_[a][i]].push_back(i);
        } else if (metrics_[a]->IsNumeric()) {
            cmp_mode_[a] = CmpMode::kNumeric;
            column_vals_[a].resize(num_rows_);
            for (size_t i = 0; i < num_rows_; ++i) {
                try {
                    column_vals_[a][i] = std::stod(col[i]);
                } catch (...) {
                    column_vals_[a][i] = std::numeric_limits<double>::quiet_NaN();
                }
            }
        } else {
            cmp_mode_[a] = CmpMode::kGeneric;
            column_lens_[a].resize(num_rows_);
            for (size_t i = 0; i < num_rows_; ++i) column_lens_[a][i] = col[i].size();
        }
    }
    LOG_INFO("Comparison modes: ids={}, numeric={}, generic={}",
             std::ranges::count(cmp_mode_, CmpMode::kIds),
             std::ranges::count(cmp_mode_, CmpMode::kNumeric),
             std::ranges::count(cmp_mode_, CmpMode::kGeneric));
}

void GaRfd::BuildAttributeBitset(size_t a) {
    BuildAttributeBitsetRange(a, 0, num_rows_);
    LOG_INFO("Finished attribute {} similarity bitset", a);
}

void GaRfd::BuildEqualityBitsetRange(size_t a, size_t i0, size_t i1) {
    auto& bits = attr_similarity_bits_[a];
    auto const& groups = equality_groups_[a];
    for (auto const& group : groups) {
        for (size_t pi = 0; pi < group.size(); ++pi) {
            size_t const p = group[pi];
            if (p < i0 || p >= i1) continue;
            for (size_t qi = pi + 1; qi < group.size(); ++qi) {
                size_t const q = group[qi];
                // q > p, so the ordered pair (p, q) is the one the naive loop sets.
                size_t const P = p * num_rows_ - p * (p + 1) / 2 + (q - p - 1);
                // Atomic like DepositBlock: parallel row-chunks write distinct bits
                // of the same word, so a plain `|=` would race between threads.
                std::atomic_ref<uint64_t>(bits[P >> 6])
                        .fetch_or(static_cast<uint64_t>(1) << (P & 63), std::memory_order::relaxed);
            }
        }
    }
}

namespace {
// Deposits a 64-bit word `w` (bit k corresponds to the k-th pair of the block,
// whose global pair index is P0 + k) into the bitset. 64 consecutive pairs span
// at most two 64-bit words, so we shift into the first word and overflow into the
// next when the block does not start on a word boundary.
//
// The writes are atomic: row-chunks of the same attribute write distinct bits but
// share 64-bit words at the chunk boundaries (and for small datasets all chunks
// fall into the same word), so a plain `|=` would race between threads.
inline void DepositBlock(std::vector<uint64_t, algos::rfd::HugePageAllocator<uint64_t>>& bits,
                         size_t P0, uint64_t w) {
    unsigned const r = static_cast<unsigned>(P0 & 63);
    size_t const w0 = P0 >> 6;
    auto constexpr kRelaxed = std::memory_order::relaxed;
    if (r == 0) {
        std::atomic_ref<uint64_t>(bits[w0]).fetch_or(w, kRelaxed);
    } else {
        std::atomic_ref<uint64_t>(bits[w0]).fetch_or(w << r, kRelaxed);
        // The low r bits of w overflow into the next word. The highest possible pair
        // index is total-1 < bits.size()*64, so when w0 is the last word the overflow
        // portion is always zero and there is no next word to write into.
        if (w0 + 1 < bits.size())
            std::atomic_ref<uint64_t>(bits[w0 + 1]).fetch_or(w >> (64 - r), kRelaxed);
    }
}
}  // namespace

void GaRfd::BuildAttributeBitsetRange(size_t a, size_t i0, size_t i1) {
    auto& bits = attr_similarity_bits_[a];
    double const min_sim = min_similarity_[a];

    // Exact-equality attributes: build from in-group pairs in O(sum group^2)
    // instead of comparing every pair (P3).
    if (cmp_mode_[a] == CmpMode::kIds) {
        BuildEqualityBitsetRange(a, i0, i1);
        return;
    }

    // Global pair index of (i, j) with j > i is
    //   P = i*num_rows_ - i*(i+1)/2 + (j - i - 1)
    // and the bit lives at word P/64, bit P%64. For a fixed row range [i0, i1)
    // this occupies a contiguous suffix of the bit vector, so chunks are disjoint.
    // The inner j loop is processed in fixed blocks of 64 pairs (P1): the per-pair
    // comparison results are gathered into one 64-bit word and deposited at once,
    // removing the per-pair mask-shift branch and letting the compiler vectorize
    // the comparisons.
    switch (cmp_mode_[a]) {
        case CmpMode::kNumeric: {
            auto const& vals = column_vals_[a];
            auto const& metric = *metrics_[a];
            for (size_t i = i0; i < i1; ++i) {
                double const vi = vals[i];
                size_t const base = i * num_rows_ - i * (i + 1) / 2 - i - 1;
                for (size_t j = i + 1; j < num_rows_; j += 64) {
                    size_t const P0 = base + j;
                    size_t const jmax = std::min<size_t>(j + 64, num_rows_);
                    uint64_t w = 0;
                    for (size_t k = 0; k < jmax - j; ++k) {
                        if (metric.NumericSatisfies(vi, vals[j + k], min_sim))
                            w |= (uint64_t(1) << k);
                    }
                    DepositBlock(bits, P0, w);
                }
            }
            break;
        }
        case CmpMode::kGeneric:
        default: {
            auto const& col = column_data_[a];
            auto const& metric = *metrics_[a];
            for (size_t i = i0; i < i1; ++i) {
                auto const& val_i = col[i];
                size_t const base = i * num_rows_ - i * (i + 1) / 2 - i - 1;
                for (size_t j = i + 1; j < num_rows_; j += 64) {
                    size_t const P0 = base + j;
                    size_t const jmax = std::min<size_t>(j + 64, num_rows_);
                    uint64_t w = 0;
                    for (size_t k = 0; k < jmax - j; ++k) {
                        if (metric.Satisfies(val_i, col[j + k], min_sim)) w |= (uint64_t(1) << k);
                    }
                    DepositBlock(bits, P0, w);
                }
            }
            break;
        }
    }
}

void GaRfd::BuildSimilarityBitsets() {
    LOG_INFO(
            "BuildSimilarityBitsets: total_pairs_ = {}, num_attrs_ = {}, num_rows_ = {}, threads = "
            "{}",
            total_pairs_, num_attrs_, num_rows_, static_cast<unsigned>(threads_));
    std::size_t const num_uint64_per_attr = (total_pairs_ + 63) / 64;

    // Exact-equality tables need no similarity bitsets: support(mask) is the
    // number of row pairs agreeing on every attribute of mask, which the direct
    // precompute below derives from the interned column ids in O(rows * 2^attrs)
    // integer work. That is strictly cheaper than the pair loop + AND-reduce for
    // every table that would have taken the precompute branch anyway, and it also
    // covers masks the old precompute bound would have left on-the-fly.
    bool const all_ids = std::ranges::all_of(
            cmp_mode_, [](CmpMode mode) { return mode == CmpMode::kIds; });
    // Bounds the direct precompute: O(rows * 2^attrs) work and ~16 bytes of
    // transient storage per entry (two flat slices), so rows * table_size = 2^22
    // keeps the build under ~30 ms and ~70 MB on one core.
    constexpr std::size_t kMaxDirectRows = std::size_t{1} << 22;
    if (all_ids && num_attrs_ < 24 && num_rows_ <= (kMaxDirectRows >> num_attrs_)) {
        BuildSupportIndexDirect();
        return;
    }
    if (all_ids) {
        // Wide exact-equality table: a full support table (2^num_attrs_ entries)
        // and the similarity bitsets are both too expensive, but the GA only ever
        // evaluates a few thousand distinct masks. Compute supports lazily per
        // queried mask (exact, cached) instead.
        lazy_support_ = true;
        LOG_INFO("Using lazy per-mask support compute ({} rows, {} attributes)", num_rows_,
                 num_attrs_);
        return;
    }

    attr_similarity_bits_.assign(num_attrs_, AttrBits(num_uint64_per_attr, 0));

    // The per-attribute pair loop is the dominant cost (~O(rows^2) metric.Compare
    // calls). Parallelize across BOTH attributes and rows: every (attribute x
    // row-chunk) task is submitted to a single pool, so all `threads_` workers stay
    // busy across attributes (they are no longer processed one at a time with a
    // per-attribute barrier). Each chunk writes a contiguous sub-range of
    // attr_similarity_bits_[a]'s bits; distinct chunks never set the same bit, and
    // the shared 64-bit words at chunk boundaries are written with atomic
    // fetch_or (see DepositBlock / BuildEqualityBitsetRange), so the build is
    // race-free.
    auto const num_threads = threads_ > 1 ? static_cast<size_t>(threads_) : size_t{1};
    if (num_threads > 1 && num_rows_ > 1) {
        std::optional<util::WorkerThreadPool> pool(threads_);
        size_t const chunk = (num_rows_ + num_threads - 1) / num_threads;
        std::size_t const total_tasks = num_attrs_ * num_threads;
        pool->ExecIndex(
                [this, chunk, num_threads](model::Index t) {
                    size_t const a = t / num_threads;
                    size_t const ck = t % num_threads;
                    size_t const i0 = ck * chunk;
                    size_t const i1 = std::min<size_t>(i0 + chunk, num_rows_);
                    if (i0 < i1) BuildAttributeBitsetRange(a, i0, i1);
                },
                static_cast<model::Index>(total_tasks));
    } else {
        for (size_t a = 0; a < num_attrs_; ++a) BuildAttributeBitset(a);
    }
    LOG_INFO("Similarity bitsets built for {} attributes", num_attrs_);

    // Precompute support for every attribute mask into a 2^num_attrs_ lookup table,
    // replacing the per-generation AND-reduce + popcount with an O(1) table read.
    // Bounded so it never blows up: the build cost is O(2^num_attrs_ * num_attrs_ *
    // words) and the table is 2^num_attrs_ * 8 bytes. Skip precompute (fall back to
    // the on-the-fly AND-reduce) when the estimated cost exceeds the threshold.
    std::size_t const words_per_attr = (total_pairs_ + 63) / 64;
    constexpr std::size_t kMaxPrecomputeOps = 1'000'000'000;
    std::size_t const table_size = static_cast<std::size_t>(1) << num_attrs_;
    bool const can_precompute = table_size <= (std::size_t{1} << 20) &&
                                table_size * num_attrs_ * words_per_attr <= kMaxPrecomputeOps;
    if (can_precompute) {
        BuildSupportIndex();
    } else {
        LOG_INFO("Skipping support precompute (num_attrs_={}, pairs={}); using on-the-fly compute",
                 num_attrs_, total_pairs_);
    }
}

void GaRfd::BuildSupportIndex() {
    std::size_t const table_size = static_cast<std::size_t>(1) << num_attrs_;
    support_index_.assign(table_size, 0);

    // Empty mask: all pairs satisfy it.
    support_index_[0] = total_pairs_;

    std::size_t const vec_size =
            attr_similarity_bits_.empty() ? 0 : attr_similarity_bits_.front().size();

    // Each mask's AND-reduce is fully independent (writes a distinct table entry),
    // so the loop parallelizes across masks. A thread-local scratch buffer avoids
    // the data race the old shared compute_buffer_ had under parallelism.
    auto reduce_mask = [this, vec_size](uint32_t mask) {
        thread_local std::vector<uint64_t> buf;
        if (buf.size() != vec_size) buf.resize(vec_size);

        uint32_t mm = mask;
        int a = FirstSetBit(mm);
        mm &= mm - 1;

        std::memcpy(buf.data(), attr_similarity_bits_[a].data(), vec_size * sizeof(uint64_t));

        bool zero = false;
        while (mm) {
            int b = FirstSetBit(mm);
            auto const& other = attr_similarity_bits_[b];
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
        std::optional<util::WorkerThreadPool> pool(threads_);
        // Index 0 is set manually above; parallelize the remaining masks [1, table_size).
        pool->ExecIndex(
                [&reduce_mask](model::Index m) { reduce_mask(static_cast<uint32_t>(m + 1)); },
                static_cast<model::Index>(table_size - 1));
    } else {
        for (uint32_t mask = 1; mask < table_size; ++mask) reduce_mask(mask);
    }
}

void GaRfd::BuildSupportIndexDirect() {
    std::size_t const table_size = static_cast<std::size_t>(1) << num_attrs_;
    support_index_.assign(table_size, 0);
    support_index_[0] = total_pairs_;
    LOG_INFO("Using direct support precompute ({} rows, {} attributes)", num_rows_, num_attrs_);

    // Flat per-mask row partitions: mask m's rows live in rows_flat[m*num_rows_,
    // (m+1)*num_rows_), and the boundaries between its groups in the slice
    // bounds_flat[m*(num_rows_+1), ...) — group g spans
    // [bounds_flat[off+g], bounds_flat[off+g+1]). The bounds slice needs
    // num_rows_+1 entries: one boundary per group plus the trailing one (a mask
    // can have up to num_rows_ groups of size 1). A mask's partition refines its
    // parent's (parent = mask without the lowest attribute): every parent group
    // is split by the added attribute's interned ids with a counting sort, so
    // the total work is O(num_rows_ * 2^num_attrs_) and needs no similarity
    // bitsets at all.
    //
    // Both arrays are backed by the huge-page-aware allocator (mmap), so their
    // memory returns to the OS when the precompute finishes. The benchmark runs
    // many datasets in one process, and plain-malloc'd per-group vectors would
    // otherwise let RSS accumulate across runs.
    std::size_t const slice = num_rows_ * table_size;
    std::size_t const bnd_slice = (num_rows_ + 1) * table_size;
    std::vector<size_t, HugePageAllocator<size_t>> rows_flat(slice);
    std::vector<size_t, HugePageAllocator<size_t>> bounds_flat(bnd_slice);
    std::vector<size_t> num_groups(table_size, 0);
    std::iota(rows_flat.begin(), rows_flat.begin() + num_rows_, size_t{0});
    bounds_flat[0] = 0;
    bounds_flat[1] = num_rows_;
    num_groups[0] = 1;

    // Masks of the same popcount are independent (they read the previous level's
    // slices and write their own), so process level by level, parallelizing each
    // level. The pool is created once and reused across levels.
    auto refine_mask = [this, &rows_flat, &bounds_flat, &num_groups](uint32_t mask) {
        uint32_t const parent = mask & (mask - 1);
        int const a = FirstSetBit(mask);
        auto const& col_ids = column_ids_[static_cast<size_t>(a)];
        std::size_t const rows_off = static_cast<std::size_t>(mask) * num_rows_;
        std::size_t const rows_p_off = static_cast<std::size_t>(parent) * num_rows_;
        std::size_t const bnd_off = static_cast<std::size_t>(mask) * (num_rows_ + 1);
        std::size_t const bnd_p_off = static_cast<std::size_t>(parent) * (num_rows_ + 1);

        // Per-worker scratch, reused across masks and groups. Sized to num_rows_
        // (an attribute has at most num_rows_ distinct interned ids); count is
        // reset for every group so count[id] == 0 marks an untouched id.
        static thread_local std::vector<uint32_t> count;
        static thread_local std::vector<uint32_t> off;
        static thread_local std::vector<uint32_t> touched;
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
            // Pass 1: count the group's rows by the added attribute's id.
            for (std::size_t i = b0; i < b1; ++i) {
                uint32_t const id = col_ids[rows_flat[rows_p_off + i]];
                if (count[id] == 0) {
                    count[id] = 1;
                    touched.push_back(id);
                } else {
                    ++count[id];
                }
            }
            // Pass 2: subgroup start offsets (in first-seen id order) + support.
            for (uint32_t id : touched) {
                std::size_t const c = count[id];
                support += c * (c - 1) / 2;
                off[id] = static_cast<uint32_t>(cursor);
                cursor += c;
                bounds_flat[bnd_off + ++child_groups] = cursor;
            }
            // Pass 3: scatter the rows into the child's slice.
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

    std::optional<util::WorkerThreadPool> pool;
    if (threads_ > 1) pool.emplace(threads_);
    for (uint32_t pc = 1; pc <= num_attrs_; ++pc) {
        std::vector<uint32_t> level;
        for (uint32_t mask = 1; mask < table_size; ++mask) {
            if (std::popcount(mask) == pc) level.push_back(mask);
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

std::size_t GaRfd::ComputeSupportDirect(uint32_t attrs_mask) const {
    // Exact support of a mask over the interned column ids: group the rows by
    // their id tuple on the mask's attributes, then support = sum over groups of
    // C(size, 2). Grouping = sorting row indices with a lexicographic comparator
    // over the ids; rows with an identical tuple are contiguous in any valid sort
    // (the comparator's equivalence is exactly "equal tuple"), so the value is
    // deterministic and there are no hash collisions to guard against. The
    // scratch buffer is thread_local, keeping parallel evaluations race-free.
    uint32_t mm = attrs_mask;
    uint32_t attrs[32];
    int k = 0;
    while (mm) {
        attrs[k++] = static_cast<uint32_t>(FirstSetBit(mm));
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

std::size_t GaRfd::ComputeSupportLazy(uint32_t attrs_mask) const {
    if (attrs_mask == 0) [[unlikely]] {
        return total_pairs_;
    }

    // Single attribute: sum over its equality groups, no sort needed.
    if ((attrs_mask & (attrs_mask - 1)) == 0u) [[unlikely]] {
        std::size_t s = 0;
        for (auto const& group : equality_groups_[static_cast<size_t>(FirstSetBit(attrs_mask))]) {
            s += group.size() * (group.size() - 1) / 2;
        }
        return s;
    }

    {
        std::lock_guard<std::mutex> const lock(support_cache_mutex_);
        auto const it = support_cache_.find(attrs_mask);
        if (it != support_cache_.end()) return it->second;
    }

    std::size_t const support = ComputeSupportDirect(attrs_mask);

    {
        std::lock_guard<std::mutex> const lock(support_cache_mutex_);
        // Bounded cache: once full, drop it entirely (recomputing is cheap and
        // the GA revisits few masks).
        if (support_cache_.size() >= cache_max_size_) support_cache_.clear();
        support_cache_.try_emplace(attrs_mask, support);
    }
    return support;
}

std::size_t GaRfd::ComputeSupport(uint32_t attrs_mask) const {
    if (!support_index_.empty()) [[likely]] {
        return support_index_[attrs_mask];
    }

    if (lazy_support_) [[unlikely]] {
        return ComputeSupportLazy(attrs_mask);
    }

    if (attrs_mask == 0) [[unlikely]] {
        return total_pairs_;
    }

    uint32_t mm = attrs_mask;
    int first = FirstSetBit(mm);
    if (first < 0) [[unlikely]] {
        return 0;
    }

    auto const& first_vec = attr_similarity_bits_[first];

    if (first_vec.empty()) [[unlikely]] {
        return 0;
    }

    if ((attrs_mask & (attrs_mask - 1)) == 0u) {
        std::size_t s = 0;
        for (uint64_t w : first_vec) s += std::popcount(w);
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
        LOG_DEBUG("Support for mask {} = {}", BitRepresentation(attrs_mask, num_attrs_), s);
#endif
        return s;
    }

    std::size_t const vec_size = first_vec.size();
    thread_local std::vector<uint64_t> buf;
    if (buf.size() != vec_size) buf.resize(vec_size);

    std::memcpy(buf.data(), first_vec.data(), vec_size * sizeof(uint64_t));

    mm &= mm - 1;
    while (mm) {
        int a = FirstSetBit(mm);
        auto const& other = attr_similarity_bits_[a];
        std::size_t running = 0;
        for (std::size_t k = 0; k < vec_size; ++k) {
            buf[k] &= other[k];
            running += std::popcount(buf[k]);
        }
        if (running == 0) [[unlikely]] {
            return 0;
        }
        mm &= mm - 1;
    }

    std::size_t support = 0;
    for (std::size_t k = 0; k < vec_size; ++k) support += std::popcount(buf[k]);

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
    LOG_DEBUG("Support for mask {} = {}", BitRepresentation(attrs_mask, num_attrs_), support);
#endif
    return support;
}

GaRfd::Individual GaRfd::Evaluate(Individual const& ind) const {
    uint32_t const lhs_mask = ind.lhs_mask;
    uint8_t const rhs = ind.rhs_index;

    double support_lhs = static_cast<double>(ComputeSupport(lhs_mask)) / total_pairs_;
    if (support_lhs == 0.0) [[unlikely]] {
        return {lhs_mask, rhs, 0.0, 0.0};
    }

    uint32_t const both_mask = lhs_mask | (1u << rhs);
    double support_both = static_cast<double>(ComputeSupport(both_mask)) / total_pairs_;
    double confidence = support_both / support_lhs;
    return {lhs_mask, rhs, confidence, support_both};
}

void GaRfd::EvaluatePopulation(Population& pop) const {
    // With a precomputed support index, Evaluate is two O(1) table reads per
    // individual, so spawning worker threads per generation only adds thread
    // startup overhead (it used to amortize expensive AND-reduce computes).
    if (support_index_.empty() && threads_ > 1 && pop.size() >= 2) {
        // Each thread writes only its own slot, and ComputeSupport reads the
        // read-only attr_similarity_bits_, so this is data-race free.
        std::atomic<model::Index> idx = 0;
        auto work = [&]() {
            model::Index i;
            while ((i = idx.fetch_add(1, std::memory_order::acquire)) < pop.size()) {
                pop[i] = Evaluate(pop[i]);
            }
        };
        std::vector<std::thread> workers;
        workers.reserve(threads_);
        for (config::ThreadNumType t = 0; t < threads_; ++t) workers.emplace_back(work);
        work();
        for (auto& w : workers) w.join();
        return;
    }
    for (auto& ind : pop) ind = Evaluate(ind);
}

bool GaRfd::AllOf(Population const& pop) const {
    return !pop.empty() && std::ranges::all_of(pop, [this](Individual const& ind) {
        return ind.confidence >= eps_;
    });
}

void GaRfd::Deduplicate(Population& pop) const {
    std::unordered_set<Individual, IndividualHash> seen;
    seen.reserve(pop.size());
    Population out;
    out.reserve(pop.size());
    for (auto& ind : pop) {
        if (seen.insert(ind).second) out.push_back(ind);
    }
    pop = std::move(out);
}

double GaRfd::Fitness(double confidence) const noexcept {
    return confidence >= eps_ ? 1.0 : confidence / eps_;
}

GaRfd::Population GaRfd::InitializePopulation(Rng& rng) const {
    Population pop;
    pop.reserve(population_size_);

    std::uniform_int_distribution<uint8_t> rhs_dist(0, num_attrs_ - 1);
    std::uniform_int_distribution<uint8_t> k_dist(1, num_attrs_ - 1);
    std::uniform_int_distribution<uint8_t> shuffle_dist;

    std::vector<uint8_t> all_indices(num_attrs_);
    std::iota(all_indices.begin(), all_indices.end(), 0);

    std::vector<uint8_t> pool(num_attrs_);
    uint8_t const effective_last = static_cast<uint8_t>(num_attrs_ - 1);

    std::size_t cnt = 0;
    while (pop.size() < population_size_ && cnt++ < population_size_ * 2 + 1) {
        uint8_t const rhs = rhs_dist(rng);
        uint8_t const k = k_dist(rng);

        std::memcpy(pool.data(), all_indices.data(), num_attrs_ * sizeof(uint8_t));

        std::swap(pool[rhs], pool[effective_last]);

        for (uint8_t i = 0; i < k; ++i) {
            using param_t = std::uniform_int_distribution<uint8_t>::param_type;
            uint8_t j = shuffle_dist(rng, param_t(i, effective_last - 1));
            std::swap(pool[i], pool[j]);
        }

        uint32_t lhs_mask = 0;
        for (uint8_t i = 0; i < k; ++i) {
            lhs_mask |= (1u << pool[i]);
        }

        pop.emplace_back(Individual{lhs_mask, rhs, 0.0, 0.0});
    }
    return pop;
}

GaRfd::Population GaRfd::Select(Population const& pop, Rng& rng) const {
    Population selected;
    selected.reserve(pop.size());

    std::uniform_real_distribution<double> dist01(0.0, 1.0);

    Individual const* best = nullptr;
    double best_confidence = -1.0;

    for (auto const& ind : pop) {
        if (ind.confidence > best_confidence) {
            best_confidence = ind.confidence;
            best = &ind;
        }

        if (dist01(rng) < Fitness(ind.confidence)) {
            selected.push_back(ind);
        }
    }

    if (selected.empty()) {
        selected.push_back(*best);
    }

    return selected;
}

GaRfd::Population GaRfd::Crossover(Population const& selected, Rng& rng) const {
    Population offspring;
    size_t const n = selected.size();
    if (n < 2) return offspring;

    offspring.reserve(std::min(n * (n - 1), static_cast<size_t>(population_size_ + 200)));

    // Bound the otherwise O(n^2) all-pairs loop. For large populations (e.g. the
    // high-eval benchmark, pop 8192) the full loop dominates runtime while most
    // pairs are skipped by the crossover probability anyway. Capping the number of
    // pair evaluations only trims redundant work; it leaves small-population runs
    // (and thus existing unit-test behaviour) completely unchanged.
    constexpr std::size_t kMaxCrossoverPairs = 4'000'000;
    std::size_t pairs_left = std::min<std::size_t>(kMaxCrossoverPairs, n * (n - 1) / 2);

    std::uniform_real_distribution<double> dist01(0.0, 1.0);
    std::bernoulli_distribution coin(0.5);

    for (auto it1 = selected.begin(); it1 != selected.end() && pairs_left > 0; ++it1) {
        auto it2 = it1;
        ++it2;
        for (; it2 != selected.end() && pairs_left > 0; ++it2, --pairs_left) {
            if (dist01(rng) >= crossover_probability_) continue;

            Individual const& p1 = *it1;
            Individual const& p2 = *it2;

            uint32_t mask1 = p1.lhs_mask;
            uint32_t mask2 = p2.lhs_mask;
            uint8_t rhs1 = p1.rhs_index;
            uint8_t rhs2 = p2.rhs_index;

            uint32_t diff = mask1 ^ mask2;
            if (diff) {
                int diff_cnt = std::popcount(diff);
                int cnt = (diff_cnt > 0) ? std::uniform_int_distribution<int>(1, diff_cnt)(rng) : 0;
                while (cnt--) {
                    uint32_t bit = diff & -diff;
                    mask1 ^= bit;
                    mask2 ^= bit;
                    diff &= diff - 1;
                }
            }

            if (rhs1 != rhs2 && coin(rng)) {
                std::swap(rhs1, rhs2);
            }

            if ((mask1 != 0) && !(mask1 & (1u << rhs1)))
                offspring.emplace_back(mask1, rhs1, 0.0, 0.0);
            if ((mask2 != 0) && !(mask2 & (1u << rhs2)))
                offspring.emplace_back(mask2, rhs2, 0.0, 0.0);
        }
    }
    return offspring;
}

GaRfd::Population GaRfd::Mutate(Population const& pop, Rng& rng) const {
    Population mutated;
    mutated.reserve(pop.size());

    std::uniform_real_distribution<double> dist01(0.0, 1.0);
    std::uniform_int_distribution<uint8_t> coin(0, 2);

    for (auto const& ind : pop) {
        if (dist01(rng) >= mutation_probability_) {
            mutated.push_back(ind);
            continue;
        }

        uint32_t mask = ind.lhs_mask;
        uint8_t rhs = ind.rhs_index;

        bool mutated_flag = false;
        switch (coin(rng)) {
            case 0: {  // Remove one random set bit from the mask
                int ones = std::popcount(mask);
                if (ones == 0) break;
                int skip = std::uniform_int_distribution<int>(0, ones - 1)(rng);
                uint32_t m = mask;
                while (skip--) m &= m - 1;
                mask ^= (m & -m);
                mutated_flag = true;
                break;
            }
            case 1: {  // Add a random available bit to the lhs (excludes bits already in lhs and
                       // rhs)
                uint32_t avail = (full_mask_) & ~mask & ~(1u << rhs);
                if (avail == 0) break;
                int ones = std::popcount(avail);
                int skip = std::uniform_int_distribution<int>(0, ones - 1)(rng);
                uint32_t m = avail;
                while (skip--) m &= m - 1;
                mask |= (m & -m);
                mutated_flag = true;
                break;
            }
            case 2: {  // Move the rhs variable to a new available bit (not in lhs and curr rhs)
                uint32_t avail = (full_mask_) & ~mask & ~(1u << rhs);
                if (avail == 0) break;
                int ones = std::popcount(avail);
                int skip = std::uniform_int_distribution<int>(0, ones - 1)(rng);
                uint32_t m = avail;
                while (skip--) m &= m - 1;
                rhs = static_cast<uint8_t>(__builtin_ctz(m & -m));
                mutated_flag = true;
                break;
            }
        }
        if (mutated_flag && mask != 0 && !(mask & (1u << rhs)))
            mutated.emplace_back(mask, rhs, 0.0, 0.0);
        else
            mutated.push_back(ind);
    }
    return mutated;
}

std::unordered_set<RFD, RFDHash> GaRfd::Finalize(Population const& pop) const {
    std::unordered_set<RFD, RFDHash> res;
    res.reserve(pop.size());

    for (auto const& ind : pop) {
        if (ind.confidence < eps_) continue;
        res.emplace(ind.lhs_mask, ind.rhs_index, ind.support, ind.confidence);
    }
    LOG_INFO("Finalized {} unique RFDs", res.size());
    return res;
}

void GaRfd::ExecuteInternal() {
    LOG_INFO("Build similarity bitsets...");
    BuildSimilarityBitsets();
    Rng rng(rng_engine_, seed_);
    auto pop = InitializePopulation(rng);
    EvaluatePopulation(pop);
    for (size_t gen = 0; gen < max_generations_; gen++) {
        LOG_INFO("Generation {}/{} (pop size: {})", gen + 1, max_generations_, pop.size());
        if (pop.size() >= population_size_ && AllOf(pop)) {
            LOG_INFO("All individuals satisfy confidence threshold - stopping early");
            break;
        }
        if (pop.empty()) [[unlikely]] {
            LOG_INFO("Population is empty, stopping evolution");
            break;
        }
        auto selected = Select(pop, rng);
        auto offspring = Crossover(selected, rng);
        auto mutated = Mutate(selected, rng);
        pop = std::move(selected);
        pop.insert(pop.end(), offspring.begin(), offspring.end());
        pop.insert(pop.end(), mutated.begin(), mutated.end());
        Deduplicate(pop);
        if (pop.size() > population_size_ + 100) {
            std::sort(pop.begin(), pop.end(),
                      [](auto const& a, auto const& b) { return a.confidence > b.confidence; });
            pop.resize(population_size_ + 100);
        }
    }
    discovered_ = Finalize(pop);
}

void GaRfd::ResetState() {
    discovered_.clear();
    support_index_.clear();
    support_cache_.clear();
    lazy_support_ = false;
}

GaRfd::~GaRfd() {
    ResetState();
}

}  // namespace algos::rfd
