#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/rfd/distance_metric.h"
#include "core/algorithms/rfd/ga_rfd/ga_rfd.h"
#include "core/config/names.h"
#include "core/parser/csv_parser/csv_parser.h"
#include "core/util/custom_metric/custom_metric.h"
#include "tests/common/all_csv_configs.h"

namespace tests {
namespace rfd = algos::rfd;
namespace util = ::util;

class GaRfdTester {
public:
    static void BuildMatchBitsets(rfd::GaRfd& algo) {
        algo.BuildMatchBitsets();
    }

    static std::size_t ComputeSupport(rfd::GaRfd const& algo, uint32_t mask) {
        return algo.ComputeSupport(mask);
    }

    static bool HasSupportIndex(rfd::GaRfd const& algo) {
        return !algo.support_index_.empty();
    }

    // Configures execute options post-LoadData, like Execute() does.
    static void Configure(rfd::GaRfd& algo, algos::StdParamsMap const& params) {
        algo.MakeExecuteOptsAvailable();
        algos::ConfigureFromMap(algo, params);
    }

    // Forces the lazy support path regardless of table shape.
    static void ForceLazyMode(rfd::GaRfd& algo) {
        algo.support_index_.clear();
        algo.lazy_support_ = true;
    }

    static void Execute(rfd::GaRfd& algo) {
        algo.ExecuteInternal();
    }
};

static algos::StdParamsMap MakeParams(config::InputTable const& table,
                                      std::vector<double> const& min_sim, double beta,
                                      std::size_t pop_size, std::size_t max_gen,
                                      config::CustomMetricsType metrics = {}) {
    algos::StdParamsMap params{{config::names::kTable, table},
                               {config::names::kRfdMinSimilarity, min_sim},
                               {config::names::kRfdMinimumConfidence, beta},
                               {config::names::kPopulationSize, pop_size},
                               {config::names::kRfdMaxGenerations, max_gen},
                               {config::names::kRfdCrossoverProbability, 0.85},
                               {config::names::kRfdMutationProbability, 0.3},
                               {config::names::kSeed, std::uint32_t{42}}};
    if (!metrics.empty()) {
        params["metrics"] = metrics;
    }
    return params;
}

TEST(GARfdSupport, SupportComputationOnIris) {
    config::InputTable table = std::make_shared<CSVParser>(kIris);

    config::CustomMetricsType metrics(5);
    for (int i = 0; i < 5; ++i) metrics[i] = rfd::EqualityMetric();

    std::vector<double> sim_vec(5, 1.0);

    auto params = MakeParams(table, sim_vec, 0.5, 10, 1, metrics);
    auto algo = algos::CreateAndLoadAlgorithm<rfd::GaRfd>(params);
    algo->Execute();

    constexpr std::size_t total_pairs = 150 * 149 / 2;

    EXPECT_EQ(GaRfdTester::ComputeSupport(*algo, 0), total_pairs);

    uint32_t species_mask = 1u << 4;
    constexpr std::size_t expected_species_support = 3 * (50 * 49 / 2);
    std::size_t actual_species = GaRfdTester::ComputeSupport(*algo, species_mask);
    EXPECT_EQ(actual_species, expected_species_support)
            << "Support for species should be 3675 (3 classes of 50)";

    uint32_t all_attrs_mask = (1u << 5) - 1;
    std::size_t all_support = GaRfdTester::ComputeSupport(*algo, all_attrs_mask);
    EXPECT_GT(all_support, 0) << "There are duplicate rows in this Iris version";

    EXPECT_EQ(GaRfdTester::ComputeSupport(*algo, species_mask), expected_species_support);
}

TEST(GARfdSupport, CacheReuse) {
    config::InputTable table = std::make_shared<CSVParser>(kIris);
    config::CustomMetricsType metrics(5, rfd::EqualityMetric());
    std::vector<double> sim_vec(5, 1.0);

    auto params = MakeParams(table, sim_vec, 0.5, 10, 1, metrics);
    auto algo = algos::CreateAndLoadAlgorithm<rfd::GaRfd>(params);
    algo->Execute();
    GaRfdTester::BuildMatchBitsets(*algo);

    uint32_t mask = 1u << 4;
    std::size_t first = GaRfdTester::ComputeSupport(*algo, mask);
    std::size_t second = GaRfdTester::ComputeSupport(*algo, mask);
    EXPECT_EQ(first, second);
}

// Support must shrink monotonically when attributes are added to a mask.
TEST(GARfdSupport, PrecomputedIndexConsistency) {
    config::InputTable table = std::make_shared<CSVParser>(kIris);
    config::CustomMetricsType metrics(5, rfd::EqualityMetric());
    std::vector<double> sim_vec(5, 1.0);

    auto algo = std::make_unique<rfd::GaRfd>();
    auto params = MakeParams(table, sim_vec, 0.5, 10, 1, metrics);
    algos::ConfigureFromMap(*algo, params);
    algo->LoadData();
    GaRfdTester::Configure(*algo, params);
    GaRfdTester::BuildMatchBitsets(*algo);

    constexpr std::size_t total_pairs = 150 * 149 / 2;
    uint32_t const full_mask = (1u << 5) - 1;

    EXPECT_EQ(GaRfdTester::ComputeSupport(*algo, 0), total_pairs);
    std::size_t const full_support = GaRfdTester::ComputeSupport(*algo, full_mask);
    EXPECT_LE(full_support, total_pairs);

    for (uint32_t mask = 0; mask < (1u << 5); ++mask) {
        std::size_t const s = GaRfdTester::ComputeSupport(*algo, mask);
        EXPECT_LE(s, total_pairs) << "mask=" << mask;
        for (int a = 0; a < 5; ++a) {
            if (mask & (1u << a)) continue;
            uint32_t const bigger = mask | (1u << a);
            EXPECT_GE(s, GaRfdTester::ComputeSupport(*algo, bigger))
                    << "mask=" << mask << " adding attr " << a;
        }
    }
}

TEST(GARfdSupport, PrecomputeModeOffSkipsIndex) {
    // Non-exact metrics take the bitset path, where the mode switch applies
    // (exact tables always use the direct/lazy paths instead).
    config::InputTable table = std::make_shared<CSVParser>(kIris);
    config::CustomMetricsType metrics(5, rfd::LevenshteinMetric());
    std::vector<double> sim_vec(5, 0.8);

    auto params = MakeParams(table, sim_vec, 0.5, 10, 1, metrics);
    params[config::names::kPrecomputeSupport] = rfd::PrecomputeMode::kOff;
    auto algo = algos::CreateAndLoadAlgorithm<rfd::GaRfd>(params);
    algo->Execute();
    EXPECT_FALSE(GaRfdTester::HasSupportIndex(*algo));
}

TEST(GARfdSupport, PrecomputeModeOnMatchesAuto) {
    auto RunMasks = [](rfd::PrecomputeMode mode) {
        config::InputTable table = std::make_shared<CSVParser>(kIris);
        config::CustomMetricsType metrics(5, rfd::LevenshteinMetric());
        std::vector<double> sim_vec(5, 0.8);

        auto params = MakeParams(table, sim_vec, 0.5, 10, 1, metrics);
        params[config::names::kPrecomputeSupport] = mode;
        auto algo = algos::CreateAndLoadAlgorithm<rfd::GaRfd>(params);
        algo->Execute();
        EXPECT_TRUE(GaRfdTester::HasSupportIndex(*algo));
        std::vector<std::size_t> supports(1u << 5);
        for (uint32_t mask = 0; mask < (1u << 5); ++mask) {
            supports[mask] = GaRfdTester::ComputeSupport(*algo, mask);
        }
        return supports;
    };

    EXPECT_EQ(RunMasks(rfd::PrecomputeMode::kAuto), RunMasks(rfd::PrecomputeMode::kOn));
}

// Lazy path must return exactly the same supports as the precomputed index.
TEST(GARfdSupport, LazySupportMatchesPrecomputed) {
    config::InputTable table = std::make_shared<CSVParser>(kIris);
    config::CustomMetricsType metrics(5, rfd::EqualityMetric());
    std::vector<double> sim_vec(5, 1.0);

    auto algo = std::make_unique<rfd::GaRfd>();
    auto params = MakeParams(table, sim_vec, 0.5, 10, 1, metrics);
    algos::ConfigureFromMap(*algo, params);
    algo->LoadData();
    GaRfdTester::Configure(*algo, params);
    GaRfdTester::BuildMatchBitsets(*algo);

    std::vector<std::size_t> precomputed(1u << 5);
    for (uint32_t mask = 0; mask < (1u << 5); ++mask) {
        precomputed[mask] = GaRfdTester::ComputeSupport(*algo, mask);
    }

    GaRfdTester::ForceLazyMode(*algo);
    for (uint32_t mask = 0; mask < (1u << 5); ++mask) {
        EXPECT_EQ(GaRfdTester::ComputeSupport(*algo, mask), precomputed[mask])
                << "mask=" << mask;
    }

    // The computed values must be cached and stable across repeated queries.
    uint32_t const mask = (1u << 4) | 2u;
    EXPECT_EQ(GaRfdTester::ComputeSupport(*algo, mask), precomputed[mask]);
    EXPECT_EQ(GaRfdTester::ComputeSupport(*algo, mask), precomputed[mask]);
}

// Full GA must agree on both support paths.
TEST(GARfdSupport, LazyModeEndToEnd) {
    auto RunAndCollect = [](bool force_lazy) {
        config::InputTable table = std::make_shared<CSVParser>(kTestLong);
        config::CustomMetricsType metrics(3, rfd::EqualityMetric());
        std::vector<double> sim_vec(3, 1.0);

        auto algo = std::make_unique<rfd::GaRfd>();
        auto params = MakeParams(table, sim_vec, 0.9, 32, 30, metrics);
        algos::ConfigureFromMap(*algo, params);
        algo->LoadData();
        GaRfdTester::Configure(*algo, params);
        GaRfdTester::Execute(*algo);
        if (force_lazy) {
            GaRfdTester::ForceLazyMode(*algo);
            GaRfdTester::Execute(*algo);
        }
        std::vector<std::string> rfds;
        for (auto const& rfd : algo->GetRfds()) rfds.push_back(rfd.ToString());
        std::sort(rfds.begin(), rfds.end());
        return rfds;
    };

    EXPECT_EQ(RunAndCollect(false), RunAndCollect(true));
}

}  // namespace tests
