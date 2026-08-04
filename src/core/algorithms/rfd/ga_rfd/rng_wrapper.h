#pragma once

#include <cstdint>
#include <random>
#include <variant>

#include "rng_engine.h"

namespace algos::rfd {

// PCG-32 (minimal, self-contained). State: 2x uint64.
class Pcg32 {
    uint64_t state_ = 0x853C49E6748FEA9BULL;
    uint64_t inc_ = 0xDA3E39CB94B95BDBULL;

    static constexpr uint64_t kMult = 6364136223846793005ULL;

    [[nodiscard]] uint32_t Next() noexcept {
        uint64_t const old = state_;
        state_ = old * kMult + inc_;
        uint32_t const xorshifted = static_cast<uint32_t>(((old >> 18u) ^ old) >> 27u);
        uint32_t const rot = static_cast<uint32_t>(old >> 59u);
        return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
    }

public:
    using result_type = uint32_t;

    static constexpr result_type min() {
        return 0;
    }

    static constexpr result_type max() {
        return UINT32_MAX;
    }

    explicit Pcg32(uint64_t seed = 0) {
        inc_ = (inc_ << 1u) | 1u;
        state_ = 0;
        (void)Next();
        state_ += seed;
        (void)Next();
    }

    result_type operator()() noexcept {
        return Next();
    }
};

// xoshiro256** (minimal, self-contained). State: 4x uint64.
class Xoshiro256 {
    uint64_t s_[4] = {0x9E3779B97F4A7C15ULL, 0xBF58476D1CE4E5B9ULL, 0x94D049BB133111EBULL,
                      0xD1B54A32D192ED03ULL};

    static uint64_t Rotl(uint64_t const x, int const k) noexcept {
        return (x << k) | (x >> (64 - k));
    }

    uint64_t Next() noexcept {
        uint64_t const result = Rotl(s_[1] * 5, 7) * 9;
        uint64_t const t = s_[1] << 17;
        s_[2] ^= s_[0];
        s_[3] ^= s_[1];
        s_[1] ^= s_[2];
        s_[3] ^= s_[0];
        s_[0] ^= t;
        s_[2] = Rotl(s_[2], 45);
        return result;
    }

public:
    using result_type = uint64_t;

    static constexpr result_type min() {
        return 0;
    }

    static constexpr result_type max() {
        return UINT64_MAX;
    }

    explicit Xoshiro256(uint64_t seed = 0) {
        // Splitmix64 seed expansion.
        uint64_t z = seed + 0x9E3779B97F4A7C15ULL;
        for (int i = 0; i < 4; ++i) {
            z = (z + 0x9E3779B97F4A7C15ULL) * 0xBF58476D1CE4E5B9ULL;
            s_[i] = (z ^ (z >> 30)) * 0x94D049BB133111EBULL;
            s_[i] ^= (s_[i] >> 27);
        }
    }

    result_type operator()() noexcept {
        return Next();
    }
};

// Type-erased uniform random bit generator wrapper. Construct from an RngEngine
// + seed, then use with any std distribution (uniform_int, bernoulli, ...).
// Reports a 32-bit result_type (like std::mt19937) so std distributions never
// observe (max()-min()+1) overflowing their result_type.
class Rng {
    std::variant<std::mt19937, Pcg32, Xoshiro256, std::minstd_rand> engine_;

    struct Caller {
        template <typename Eng>
        uint32_t operator()(Eng& eng) const {
            return static_cast<uint32_t>(eng());
        }
    };

public:
    using result_type = uint32_t;

    static constexpr result_type min() {
        return 0;
    }

    static constexpr result_type max() {
        return UINT32_MAX;
    }

    Rng(RngEngine engine, uint32_t seed) {
        uint64_t const s = static_cast<uint64_t>(seed);
        switch (engine) {
            case RngEngine::kMt19937:
                engine_ = std::mt19937(seed);
                break;
            case RngEngine::kPcg32:
                engine_ = Pcg32(s);
                break;
            case RngEngine::kXoshiro256:
                engine_ = Xoshiro256(s);
                break;
            case RngEngine::kMinstdRand:
                engine_ = std::minstd_rand(seed);
                break;
        }
    }

    result_type operator()() noexcept {
        return std::visit(Caller{}, engine_);
    }
};

}  // namespace algos::rfd
