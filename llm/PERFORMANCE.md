# PERFORMANCE.md — Performance Optimization Checklist

How to speed up data-profiling algorithms (FD/UCC/IND/OD/AFD/MD discovery,
validation, statistics) in this C++ project. Read this when asked to improve
performance. Order of impact: **algorithmic wins (up to 100×+) > C++
hygiene/copies (~6–50×) > containers (~1.5–6×) > memory layout, then SIMD >
parallelization (highly variable, can be negative).**

**Scope rule:** performance work only when the user explicitly asks, and
only for what they asked — apply the checklist to that segment only, never
the whole codebase. Never optimize proactively.

## 1. Measurement foundation (first, non-negotiable)

Profile first — the GFD-mining 100×+ win was only found because someone
profiled.

- [ ] Baseline benchmark suite: fixed datasets (small/large, sparse/dense,
      few/many columns, high/low cardinality, unique/duplicate/null/string-
      heavy, narrow/wide); fixed random seeds; immutable inputs
- [ ] Define performance targets before optimizing (max wall time, peak
      memory, throughput rows/s and cells/s, scaling)
- [ ] Measurement protocol: `llm/PLAN.md` §5 (pinned frequency, isolated
      cores, caches dropped, `perf stat -r 10`, median). Never compare runs
      from different machine states
- [ ] Separate parsing vs algorithm timing; cold-cache vs warm-cache runs
- [ ] Measure memory too: peak RSS, allocation count/bytes (heaptrack/massif)
- [ ] Record every experiment (machine, compiler, flags, dataset, before/
      after, date) in `bin/measurements_<YYYY-MM-DD>.md` with the machine
      state
- [ ] Profile build: Release `./build.sh -p -b -j4 -f` — no debug symbols,
      no sanitizers (both consume memory and slow things); never benchmark
      sanitizer builds. `-j` = CNT_CPU_CORE / 2 computed on the user's PC
- [ ] Verify correctness before and after every change

### Tools
| Tool | What it finds |
|---|---|
| `perf record -F 99` + `perf report` | hot functions (use `-F 99` or don't record — the default rate bloats perf.data to GB) |
| `perf script` + FlameGraph | visual call towers ("one massive tower = your bottleneck") |
| `perf stat -ddd` | cache misses, branch mispredictions, TLB, IPC, bandwidth (no perf.data) |
| `~/pmu-tools/toplev.py` | bottleneck class (TMA) |
| `objdump -d` / `-fopt-info-vec` (`-Rpass=vector`) | verify an optimization landed / loop vectorized |
| `valgrind --tool=callgrind` + `kcachegrind` | call counts, instruction costs |
| `valgrind --tool=memcheck` | memory errors (leaks, invalid reads/writes) |
| `valgrind --tool=helgrind` / `drd` | data races, deadlocks |
| heaptrack / massif | allocation hotspots, peak memory |
| ASan/UBSan/MSan/LSan/TSan | correctness only — never for benchmarks |
| VTune / `perf lock` | contention, lock profiling |

Classify the bottleneck before changing anything: CPU / memory-bandwidth /
cache-latency / allocation / I/O / lock / branch-prediction — each has a
different fix. Check `perf stat -ddd` numbers first (cache misses → §4,
low IPC → §5, bandwidth → §4, allocations → §4).

## 2. Algorithmic optimizations (the 100× territory)

No SIMD, parallelism, or containers rescue a fundamentally wasteful
algorithm.

- [ ] **Do less work first**: reduce the search space before validating
- [ ] Prune with every mathematically valid rule (minimality,
      non-triviality, monotonicity/anti-monotonicity, known-valid/invalid
      subsets and supersets); never generate supersets of valid minimal
      keys or known determinants
- [ ] Re-read the paper + follow-up variants; verify every pruning rule it
      defines is actually implemented
- [ ] Detect constant and unique columns once, early; prune candidates they
      make trivial; drop exact-duplicate columns when appropriate
- [ ] **Sample-first validation** (HyFD): reject against a small random
      sample before full partitions; verify survivors exactly — sampling as
      a filter, never the final answer
- [ ] **Stripped partitions**: drop singleton equivalence classes everywhere
- [ ] **Refine, don't rebuild**: derive π(X∪A) from a parent partition
- [ ] **Cache expensive reusable results** with a reuse-likelihood policy
      (intersections, error metrics, cardinalities, closures, sorted
      orders, dictionaries, prefix hashes)
- [ ] Early termination: stop at the first counterexample; stop error
      accumulation once the threshold is exceeded (lower bound first)
- [ ] Order candidates by cost: ascending cardinality/selectivity,
      likely-to-fail first; high distinct counts early for uniqueness, low
      for dependency elimination
- [ ] Canonical candidate representation (sorted IDs/bitsets); deduplicate
      candidate lists; deterministic order for reproducible runs
- [ ] Level-wise processing: level k+1 only from valid level-k candidates;
      prefix grouping; release finished-level structures
- [ ] Subset-check representation: bitsets for moderate column counts,
      sorted vectors for sparse/large; cache repeated checks
- [ ] Hitting-set/dualization for sparse search spaces; incremental
      computation everywhere (extend, don't restart)
- [ ] Algorithm by table shape: sampler-based (HyFD) for wide tables,
      level-wise (TANE/Pyro) for narrow
- [ ] Check complexity constants — O(n log n) with huge constants loses on
      real data

## 3. Per-problem specifics

**FD/AFD:** integer-encoded equivalence classes, never full rows in
partitions; compact clusters (row-ID ranges / CSR). Best parent partition
for refinement. **Generation counters** instead of clearing O(n) arrays.
Cardinality proofs: |π_X| = |π_X∪A| ⇒ FD; unique X ⇒ all X→A; constant A ⇒
all X→A. Consistent null semantics in discovery and validation. Incremental
error metrics, integer counts in inner loops (ratios only at reporting),
overflow-safe counters. Prune implied FDs via cached closures. AFD:
threshold early stop, lower bounds before exact values, sampling only as
filter.

**UCC:** single-column UCCs first; cardinality lower bounds before tuple
hashes; early termination at the first duplicate; never scan singleton
partitions; reuse tuple buffers; benchmark hash vs sort + adjacent
comparison; keep only minimal UCCs, prune supersets.

**IND:** column dictionaries/sorted unique lists once (never re-read raw
values per target); cheap rejections first (distinct counts, type domain,
min/max, then merge-like scan); Bloom as a *negative* filter only; N-ary:
candidates only from compatible unary INDs, canonical ordering, selective
pairs first, prune failed projections.

**OD/sorting:** cached sorted row orders per column — sort row IDs, never
full values; radix sort for fixed-width integer IDs; branch-light, no
locale-aware comparators. Validation: cheap type/null checks first, one
linear monotonicity pass, adjacent comparisons, early stop on enough
violations, reuse group boundaries.

**Statistics:** all stats (count/null/min/max/distinct) in one scan;
numerically stable one-pass moments; shared lazy stats cache; HLL/top-k
only with documented error bounds; dense arrays for low cardinality, sparse
maps for high. Pattern inference: cheap prefilters before regex,
deterministic scanners for common patterns, incremental type inference that
stops checking eliminated candidates.

## 4. Data structures, memory layout, allocations

DFD lesson: AVX2 on heap-scattered `dynamic_bitset` objects gave ~3% (the
prefetcher couldn't help); Faida got 1.6–1.7× from layout/bufferization
alone. **Fix layout before SIMD or parallelism.**

- [ ] Flatten pointer-chasing structures into contiguous arrays;
      arena/pool-allocate hot objects (lattice vertices, clusters, trie
      nodes)
- [ ] Many small bitsets in one flat buffer + index, not N heap
      allocations; track lengths to skip unused trailing words
- [ ] SoA over AoS (this is a columnar project); PLI clusters: single flat
      `std::vector<int>` + offsets (CSR) vs deque-of-vectors — evaluate for
      your access pattern
- [ ] 64-byte alignment for hot buffers; `alignas(64)` per-thread counters
      (false sharing); compact flags arrays instead of scattered `bool`s
- [ ] `reserve()` when size is known (not premature); `emplace_back`;
      `clear()` + reuse instead of reconstructing
- [ ] Arena/scratch buffers for per-iteration temporaries; reset between
      phases
- [ ] Smallest safe integer types (`uint32_t`/`uint16_t`); missing-value
      sentinel outside the valid dictionary-ID range
- [ ] **Cache layer** where profiling shows repeated identical work — small
      reuse-likelihood/LRU (intersections, error metrics, cardinality,
      statistics); measure hit rate, keep only if it pays
- [ ] Huge pages (`madvise(MADV_HUGEPAGE)`) on multi-GB structures —
      benchmark before adopting
- [ ] Try jemalloc/mimalloc/tcmalloc in the benchmark build only; revert
      if no gain
- [ ] Indexes: permutation/rank arrays over values — sort row IDs, not
      rows; avoid materializing copies
- [ ] Cache-friendly chunking (L2/L3 blocks, inner loop over the whole
      chunk); hot/cold field splitting
- [ ] Roaring/compressed bitmaps for sparse large row sets if deps permit —
      benchmark first
- [ ] Check allocation count/page faults before/after — cheap win

## 5. Microarchitectural and SIMD

- [ ] Auto-vectorization first: simple loops, contiguous access, no virtual
      calls; verify with `-fopt-info-vec` / `-Rpass=vector`
- [ ] **Bit manipulation is prioritized over SIMD**: word-level tricks on
      `uint64_t` — subset check `(a & b) == a`, intersection/union/
      difference via `& | ^`, `std::popcount`/ctz/rotl builtins, bit-packing
      to halve memory. Portable; intrinsics only when these aren't enough
- [ ] SIMD only after layout is fixed, on contiguous buffers (a magnifier,
      not a magic wand); runtime dispatch (`__builtin_cpu_supports`) or
      `#ifdef` fallbacks — binaries ship to unknown hardware; benchmark on
      real datasets, never synthetic loops
- [ ] **Branchless code is prioritized**: conditional arithmetic, bitmasks,
      `count += (x == y)`; sort/group first to make branches predictable
- [ ] `[[likely]]`/`[[unlikely]]` on strongly skewed branches only;
      `__restrict__` on non-aliasing buffers; hoist loop invariants
- [ ] `__builtin_prefetch` only if profiling shows the misses; no manual
      unrolling — let the compiler do it

Tricks (small, targeted, easy to revert): avoid division in hot loops
(multiply + shift by a precomputed reciprocal); loop fusion; `constexpr`
lookup tables; lazy evaluation; fast-path/slow-path separation for common
data shapes (unquoted CSV, no nulls, sorted input).

## 6. Parallelization (precise, per-dataset)

Data-dependent results: DFD per-column lattices 4 threads 3.4× best / slower
worst (needs ≥ ~20 columns; tiny tasks → overhead); HyUCC Validator 4
threads 3.1×/0.8× (only when validator ≈ 90% of runtime); HyUCC + Sampler
6.8×/0.5× (some datasets 30–45% slower); ORDER 8 threads 1.14×/1.00× (early
exit → nothing to parallelize); Faida 16 threads 2.6×/slower (mutex
contention; lock-free atomics fixed it).

- [ ] Amdahl first: S = 1/((1-p) + p/N); small parallelizable fraction →
      skip this phase
- [ ] Profile for the parallelizable fraction, not just hotspots;
      granularity: tasks ≥ ~100µs; fuse tiny tasks; minimum-work threshold
- [ ] One shared thread pool, never per-task threads; configurable count,
      default conservative; work-stealing for uneven costs, static for
      uniform scans
- [ ] Per-thread buffers merged at the end > mutex-protected shared
      structures; `thread_local` counters padded; atomics only for coarse
      progress
- [ ] Watch memory bandwidth — partition intersections saturate on many
      cores; NUMA: first-touch large buffers on the using node
- [ ] Deterministic output (sorted merge of per-thread results); handle
      cancellation/exceptions without corrupting shared state
- [ ] Accept only if no significant regression anywhere on many datasets
      (project rule)

## 7. I/O and parsing

- [ ] Measure parsing separately; fix it if it dominates user-visible time
- [ ] Buffered reads / memory-mapped input; large buffers;
      `posix_fadvise` for sequential access
- [ ] Zero-copy `std::string_view` over the raw buffer; never a
      `std::string` per cell
- [ ] `std::from_chars` / fast_float; never `stoi/stod` in loops; no regex
      in hot paths
- [ ] Fast path for common unquoted/ASCII fields; quoted/malformed handling
      correct but cold; defer diagnostics until an actual error
- [ ] Dictionary-encode / intern strings once at load — algorithms compare
      integer IDs (critical for PLI construction)
- [ ] Incremental type inference: eliminate candidates as rows are scanned;
      dispatch on column type, not cell type
- [ ] For repeated runs (dynamic validation): reuse the loaded relation

## 8. Containers and hashing

Real cases: ORDER — `std::unordered_set` → `boost::unordered_flat_set` up to
×4.95; Faida — `std::unordered_map` → emhash ~1.5× everywhere.

- [ ] Profile to find the actual hot container — don't guess
- [ ] Node-based → open addressing: `ankerl::unordered_dense`,
      `absl::flat_hash_map`, `boost::unordered_flat_map`, `tsl::robin_map`,
      emhash
- [ ] `std::map`/`set` → sorted vector + binary search; `std::list` →
      vector/deque; `std::vector<bool>` → explicit bitsets (project
      `Vertical` already is)
- [ ] Match the container to the access pattern (read- vs write-heavy);
      dense integer domains → vector indexed by ID
- [ ] Keys: packed fixed-width integers, sorted ID vectors, or bitsets —
      never concatenated strings ("v1|v2|v3"), never a new allocated key
      object per row
- [ ] Incremental hashing for candidate extensions; cache prefix hashes;
      reserve before bulk insert; reuse with `clear()`; tune load factor
      only after measurement
- [ ] Benchmark hash-based grouping vs sort-and-scan (sort often has better
      locality for large candidates)
- [ ] Hash quality: xxHash/wyhash if the hash is the bottleneck; stable
      bitset-key hash when a bitset is a map key

## 9. Large data / external memory

- [ ] Detect estimated usage over a configurable budget; memory-budget
      option; chunked/streaming for incremental algorithms; two-pass where
      it reduces pressure
- [ ] External sorting; spill intermediate partitions only when necessary
- [ ] Sequential disk access, batched writes, compact binary temp formats
      (never serialize pointer-rich in-memory structures); configurable
      temp location; clean up temp files safely
- [ ] Avoid excessive page faults from mapping too much at once

## 10. Output and API

- [ ] No result formatting until discovery is complete: store numeric
      column IDs, resolve names only at serialization
- [ ] No string concatenation in loops; buffered output; no flush per
      result
- [ ] Result limits / pagination / streaming for massive result sets; make
      deterministic sort optional if it dominates
- [ ] Measure serialization (e.g. JSON) separately from discovery

## 11. Runtime observability (cheap internal counters)

Temporary counters for a profiling run (LLM can add them temporarily):
candidates generated/pruned/validated; partition construction/refinement
counts, cache hits vs misses; allocation count/bytes; hash-table ops, sort
time, per-thread idle. Keep them cheap (increments, no logging/formatting);
gate behind a debug flag.

## 12. Build configuration

- [ ] Release on (`-O3 -DNDEBUG`, `CMAKE_BUILD_TYPE=Release` — verify not
      silently Debug); LTO (`-flto`, thin for faster builds) — measure;
      `-fno-plt` helps hot loops
- [ ] `-march=native` for local builds; portable baseline
      (`-march=x86-64-v3`) for shipped binaries; `-mtune` only where
      portability is guaranteed
- [ ] Profiling builds: `-g -fno-omit-frame-pointer` (needed for perf stacks)
- [ ] **No PGO, BOLT, or any multi-training-run optimization — project
      policy** (profiling allowed; profile-guided *compilation* is not)
- [ ] Compare GCC vs Clang on the workload if a change is borderline;
      alternative allocator only as an experiment; keep exceptions/RTTI as
      the project uses them

## 13. Verification and rules

- [ ] Optimize measured bottlenecks, not guessed ones
- [ ] Correctness before and after: the algorithm's targeted tests
      (`ctest --test-dir build -R "<algo>"`, `llm/DEVELOPMENT.md` §2);
      ASan/UBSan are CI-only — never benchmark them; no sanitizer builds
      locally
- [ ] Differential testing vs a slower reference on small datasets; fuzz
      parsers and candidate generation
- [ ] Edge cases: empty, one row/column, all-null/duplicate/unique, long
      strings, malformed CSV, Unicode, integer limits
- [ ] Exact agreement between sequential and parallel paths; deterministic
      output preserved
- [ ] Approximation only as an explicit, configurable mode with documented
      error bounds and seed in result metadata — never silent
- [ ] One controlled change per experiment: profile → hypothesize →
      smallest fix → re-benchmark → correctness → keep only if real
- [ ] Re-profile after every merged change; performance work is iterative
- [ ] **Final check of every perf task: per `llm/PLAN.md` §2 — merge all
      new branches back into the branch where the user called you, then
      run the **targeted tests** (`ctest --test-dir build -R "<algo>"`;
      `llm/DEVELOPMENT.md` §2 — never the full suite) plus `valgrind`
      (memcheck), `helgrind`, and `drd`** — memory errors, data races,
      deadlocks. These are the penultimate and last tasks of the perf
      todo file

## Appendix A. Priority cheat-sheet

1. **Profile** (perf stat/record -F 99 + flamegraph + toplev) → find the
   real bottleneck class
2. **Fix the algorithm** (pruning, caching, incremental work, sampling) —
   up to 100×+
3. **Fix copies** (const&, move, reserve, views) — up to 6–50×
4. **Fix containers** (flat hash maps matched to access pattern) — 1.5–6×
5. **Fix memory layout** (contiguous, chunked, arena, CSR) — enables
   everything below
6. **Parallelize** with correct granularity — 0.5× to 6.8×, data-dependent
7. **SIMD** last, on contiguous data only — typically 1.05–1.3× on top

## Appendix B. Quick per-change review questions

- Does it add allocation, copies, virtual dispatch, locks, logging, or
  formatting to a hot loop? Work per row or per candidate?
- Does it increase asymptotic complexity, peak memory, or allocation count?
- Does it introduce a global lock or invalidate caches (partition/
  statistics reuse) too often?
- Does it preserve pruning correctness, null semantics, exactness
  guarantees, determinism?
- Was it benchmarked with the PLAN.md §5 protocol, before and after, on
  representative datasets?
