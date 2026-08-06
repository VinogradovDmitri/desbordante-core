# PERFORMANCE.md — Performance Optimization Checklist

How to speed up data-profiling algorithms (FD/UCC/IND/OD/AFD/MD discovery,
validation, statistics) in this C++ project. Read this when asked to improve
performance. Grounded in the project's own optimization history (50+ student
theses, `docs/papers`) and in what makes HyFD/HyMD-class algorithms fast.

Work top to bottom. The proven order of impact:

**algorithmic wins (up to 100×+) > C++ hygiene/copies (~6–50×) > container
choice (~1.5–6×) > memory layout, then SIMD > parallelization (highly
variable, can be negative).**

**Scope rule:** performance work is done only when the user explicitly asks
to increase performance — and only for what they asked. If the request names
a particular part of the code (a function, an algorithm, a pipeline stage),
apply this checklist to that segment only: not to the whole codebase, not to
all algorithms, not to adjacent code. Never optimize proactively; a
functional change that "also makes things faster" is fine, but this checklist
is not a justification for unrequested refactors or tuning.

## 1. Measurement foundation (do this first, non-negotiable)

The #1 lesson: profiling is the first step, not a last resort. The GFD-mining
100×+ win was only found because someone profiled first.

- [ ] Establish a baseline benchmark suite: fixed datasets covering small/
      medium/large, sparse/dense, few/many columns, high/low cardinality,
      unique-heavy, duplicate-heavy, string-heavy and null-heavy tables, both
      narrow and wide shapes. Use fixed random seeds for generated data; keep
      inputs immutable.
- [ ] Define performance targets before optimizing: max wall time per
      algorithm, max peak memory, throughput (rows/s and cells/s), scaling
      expectations in rows and columns, interactive vs batch budgets.
- [ ] Follow the measurement protocol in `llm/PLAN.md` §5 (pinned CPU
      frequency, isolated cores, caches dropped, `perf stat -r 10`, median
      reporting). Never compare runs taken under different machine states.
- [ ] Separate timing: parsing vs algorithm execution (report both). Separate
      cold-cache and warm-cache runs.
- [ ] Measure memory too: peak RSS, allocation count and total bytes
      allocated (heaptrack / massif).
- [ ] Record every experiment (machine, compiler version, flags, dataset,
      before/after numbers, date).
- [ ] Build with `-O2/-O3 -g -fno-omit-frame-pointer` for profiling; keep the
      normal Release build separate. Never benchmark sanitizer builds.
      The measurement build is `./build.sh -p -b -j4 -f` — a Release build
      with no debug symbols and no sanitizers, because both consume memory and
      decrease performance (see `llm/DEVELOPMENT.md` §1). Parallelism follows
      the same rule as every build: CNT_CPU_CORE / 2, physical cores only —
      hyper-threads excluded, E-cores counted. First task on a user PC:
      recompute CNT_CPU_CORE there before building.
- [ ] Verify correctness before and after every change (tests, sanitizers).

### Tools available on this machine

| Tool | What it finds |
|---|---|
| `perf record -F 99` + `perf report` | Hot functions, self vs inclusive time |
| `perf script` + FlameGraph scripts | Visual call towers ("one massive tower = your bottleneck") |
| `perf stat -ddd` | Cache misses, branch mispredictions, TLB misses, IPC, memory bandwidth (no perf.data) |
| `~/pmu-tools/toplev.py` | Bottleneck class: retiring / frontend / bad speculation / memory-bound (TMA) |
| `objdump -d` | Verify an optimization actually landed in the generated assembly |
| `-fopt-info-vec` (GCC) / `-Rpass=vector` (Clang) | Whether loops auto-vectorized |
| `valgrind --tool=callgrind` + `kcachegrind` | Call counts, instruction-level costs |
| `valgrind --tool=memcheck` | Memory errors (leaks, invalid reads/writes) |
| `valgrind --tool=helgrind` | Data races, deadlocks (happens-before analysis) |
| `valgrind --tool=drd` | Data races, deadlocks (lockset-based analysis) |
| `heaptrack` / `massif` | Allocation hotspots, peak memory |
| ASan / UBSan / MSan / LSan / TSan | Correctness: races, leaks, UB — never for benchmarks |
| VTune / `perf lock` | Contention, lock profiling |

### Recording rules

- [ ] Record every measurement in `bin/measurements_<YYYY-MM-DD>.md` together
      with the machine state (governor, pinned frequency, isolated cores) —
      numbers are comparable only within the same protocol state
      (`llm/PLAN.md` §5).
- [ ] **If you use `perf record`, use `-F 99` (99 Hz sampling) — or don't
      record at all.** Algorithms can run 10+ minutes; the default sampling
      rate makes `perf.data` grow to gigabytes. Use `perf stat -ddd`
      (aggregated counters, no perf.data) whenever you only need summary
      numbers, and reserve `perf record` for targeted short runs.
- [ ] Classify the bottleneck before changing anything: CPU-bound,
      memory-bandwidth-bound, cache-latency-bound, allocation-bound,
      I/O-bound, lock-bound, or branch-prediction-bound. Each has a different
      fix.
- [ ] Check `perf stat -ddd` numbers: high cache-miss rate → fix layout
      (§4); low IPC → branch stalls or dependency chains (§5); high
      bandwidth → reduce data movement (§4); allocations → §4.
- [ ] Use `objdump -d` (or Compiler Explorer) on hot loops to confirm the
      compiler did what you think — e.g. vectorized, inlined, no hidden
      copies. If the optimization isn't visible in assembly, it didn't
      happen.

## 2. Algorithmic optimizations (the 100× territory)

No amount of SIMD, parallelism, or container tuning rescues a fundamentally
wasteful algorithm.

- [ ] **Do less work first.** Reduce the search space before validating.
- [ ] Prune candidates with every mathematically valid rule before evaluating
      them: minimality, non-triviality, monotonicity/anti-monotonicity,
      known-valid/invalid subsets and supersets. Never generate supersets of
      already-valid minimal keys (UCC) or of known dependency determinants.
- [ ] Re-read the paper your algorithm implements — plus follow-up variants —
      and verify every pruning rule it defines is actually implemented.
- [ ] Detect constant columns and unique columns once, early; prune all
      candidates they make trivial. Drop exact-duplicate columns when
      appropriate.
- [ ] **Sample-first validation** (the HyFD idea): reject candidates against
      a small random sample before touching full partitions; verify survivors
      exactly. Use sampling as a filter, never as the final answer.
- [ ] Use **stripped partitions**: drop singleton equivalence classes
      everywhere (duplicate detection never needs them).
- [ ] **Refine, don't rebuild**: derive π(X∪A) from an existing parent
      partition instead of re-sorting/hashing raw rows for every candidate.
- [ ] **Cache expensive reusable results** with a reuse-likelihood policy, not
      a coin flip: partition intersections, error metrics (g1, μ+, τ),
      cardinalities, closures, sorted orders, dictionaries, prefix hashes.
- [ ] Early termination: stop validation at the first counterexample; stop
      error accumulation once the threshold is exceeded (compute a lower
      bound first, exact value only if needed).
- [ ] Order candidates by expected cost: validate by ascending cardinality /
      selectivity; process likely-to-fail candidates first; process columns
      with high distinct counts early for uniqueness, low distinct counts
      early for dependency elimination.
- [ ] Represent candidates canonically (sorted attribute IDs or bitsets) to
      prevent duplicates; deduplicate candidate lists after generation;
      prefer deterministic order for reproducible runs.
- [ ] Level-wise processing: generate level k+1 only from valid joinable
      level-k candidates; use prefix grouping to cut candidate-join
      comparisons; release data structures of finished levels instead of
      retaining the whole lattice.
- [ ] Benchmark subset-check representation: bitsets for moderate column
      counts, sorted vectors for sparse/large column counts; cache repeated
      subset-check results.
- [ ] Use hitting-set / dualization reasoning where the search space is
      sparse; skip brute-force lattice searches.
- [ ] Incremental computation everywhere: extend, don't restart.
- [ ] Choose the algorithm by table shape: lattice width 2^n explodes with
      columns — sampler-based approaches (HyFD-style) for wide tables,
      level-wise (TANE/Pyro-style) for narrow ones.
- [ ] Check complexity constants: an O(n log n) with huge constants loses to
      O(n²) on real data. Measure, don't assume.

## 3. Per-problem specifics (FD / UCC / IND / OD / statistics)

### FD and AFD discovery

- [ ] Build equivalence classes from integer-encoded values; never store full
      rows in partitions; represent clusters compactly (row-ID ranges /
      CSR-style).
- [ ] Choose the best parent partition for refinement; reuse cluster-ID
      buffers across iterations.
- [ ] Use **generation counters** instead of clearing large O(n) arrays when
      only a small subset of rows was touched.
- [ ] Exploit cardinality proofs: if |π_X(R)| = |π_{X∪A}(R)| the FD holds; if
      X is unique, X→A holds for every A; if A is constant, every X→A holds.
- [ ] Apply the same null semantics consistently in discovery and validation
      (do nulls participate in equivalence classes?).
- [ ] Error metrics: make error calculation incremental when extending
      determinants; use integer counts in inner loops (no floating point),
      compute ratios only at reporting; use overflow-safe counters.
- [ ] Use discovered FDs to prune implied candidates (closure computation,
      cached closures).
- [ ] For AFDs: threshold-based early stop; lower bounds before exact error
      values; sampling only as a filter with exact verification of survivors.

### UCC discovery

- [ ] Detect single-column UCCs first; estimate lower bounds on required
      combination size from cardinality; rule out candidates by cardinality
      bounds before touching tuple hashes.
- [ ] Validate with early termination: the first duplicate rejects the
      candidate; stop refining once all classes become singletons; never
      scan singleton partitions.
- [ ] Reuse tuple buffers across candidate checks; benchmark hash-based
      duplicate detection against sort + adjacent comparison for large
      candidates.
- [ ] Retain only minimal UCCs; prune supersets of discovered UCCs.

### IND discovery

- [ ] Compute column dictionaries/sorted unique lists once; reuse them for
      every candidate (never re-read raw values per target column).
- [ ] Cheap rejections first: source distinct count > target count,
      type-domain incompatibility, min/max range summaries, then the
      merge-like scan over sorted lists (benchmark against hash-set
      membership).
- [ ] Bloom filters as a *negative* filter only, never final proof.
- [ ] N-ary: generate candidates only from compatible unary INDs; canonical
      attribute ordering; process selective pairs first; prune on failed
      projections.

### OD / sorting-heavy algorithms

- [ ] Cache sorted row orders per column; sort row IDs, never copies of full
      values; indirect sort for large values.
- [ ] Consider radix sort for fixed-width integer IDs; keep comparators
      branch-light; avoid locale-aware and string-dereferencing comparators.
- [ ] OD validation: cheap type/null checks first, monotonicity in one linear
      pass after sorting, adjacent comparisons only, early stop on enough
      violations, reuse group boundaries.

### Statistics, histograms, patterns

- [ ] Combine count / null count / min / max / distinct in one column scan;
      never scan once per statistic; numerically stable one-pass moments.
- [ ] Share a stats cache across algorithms; make expensive statistics lazy.
- [ ] Approximate cardinality (HyperLogLog) and top-k only with clearly
      documented error bounds; dense frequency arrays for low-cardinality
      domains, sparse maps for high-cardinality.
- [ ] Pattern inference: cheap prefilters before any regex; deterministic
      scanners for common patterns (integer, date, email, UUID); incremental
      type inference that stops checking eliminated candidates.

## 4. Data structures, memory layout, and allocations

Real case: the DFD SIMD setback — AVX2 on heap-scattered `dynamic_bitset`
objects gave ~3% because the prefetcher couldn't help. Faida got 1.6–1.7×
from layout/bufferization alone, SIMD only helped on top. **Fix layout before
SIMD or parallelism.**

- [ ] Flatten pointer-chasing structures into contiguous arrays; arena- or
      pool-allocate hot objects (`LatticeVertex`, clusters, trie nodes).
- [ ] Store many small bitsets in one flat buffer with an index, not N
      separate heap allocations; track bitset length to avoid scanning
      unused trailing words.
- [ ] Structure-of-Arrays over Array-of-Structures for column processing;
      this project is columnar by nature — keep it that way.
- [ ] PLI clusters: evaluate a single flat `std::vector<int>` with offsets
      (CSR-style) against the current deque-of-vectors for your access
      pattern. Keep iterating contiguous memory in natural order.
- [ ] Align hot buffers to 64 bytes; pad per-thread counters to avoid false
      sharing (`alignas(64)`); avoid `bool` fields scattered across hot
      structs — use compact flags arrays.
- [ ] `reserve()` when size is known — reserve is not premature optimization.
      `emplace_back` over `push_back` for non-trivial types; `clear()` +
      reuse instead of reconstructing containers.
- [ ] Arena/scratch buffers for per-iteration temporaries; reset arenas
      between phases.
- [ ] Use the smallest safe integer type: `uint32_t` instead of `size_t`,
      `uint16_t` where bounds are guaranteed; keep the missing-value sentinel
      out of the valid dictionary-ID range.
- [ ] **Try implementing a cache** where profiling shows repeated identical
      work — the §2 caching policy applied at the data-structure level: the
      same partition intersection, error metric, cardinality, or statistics
      queried many times. A small reuse-likelihood or LRU layer can remove
      the work entirely; measure hit rate and keep it only if it pays.
- [ ] Use **huge pages**: `madvise(MADV_HUGEPAGE)` on multi-GB structures
      (PLI/partition buffers, big bitset arenas). TLB misses drop, but
      allocation and paging behavior change — benchmark before adopting.
- [ ] **Try different mallocs**: jemalloc / mimalloc / tcmalloc often help
      allocation-heavy phases (lattice vertices, clusters, hash tables).
      Swap the allocator in the benchmark build only, measure, revert if no
      gain (see §12).
- [ ] **Use indexes**: permutation / rank index arrays over values instead of
      moving data — sort row IDs, not rows; reference columns by index; avoid
      materializing copies of values that only need addressing.
- [ ] Cache-friendly chunking: process data in blocks that fit L2/L3; run the
      inner loop over the whole chunk.
- [ ] Hot/cold field splitting: keep frequently accessed metadata together,
      away from rarely-read fields.
- [ ] Roaring bitmaps or compressed bitmaps for sparse, large row sets if
      dependencies permit; benchmark before adopting.
- [ ] Check allocation count before/after; a drop in `perf stat` page faults
      is a cheap win.

## 5. Microarchitectural and SIMD

- [ ] Enable compiler auto-vectorization first: simple loops, contiguous
      access, no virtual calls, no complex branches; verify with
      `-fopt-info-vec` / `-Rpass=vector`.
- [ ] **Bit-manipulation is prioritized** over SIMD: word-level tricks on
      `uint64_t` words — subset check `(a & b) == a`, intersection / union /
      difference via `& | ^`, `std::popcount` / ctz / rotl builtins,
      bit-packing to halve memory footprint. Portable, often wins alone;
      reach for intrinsics only when these are not enough.
- [ ] SIMD only after layout is fixed, and on contiguous buffers only — it's
      a magnifier, not a magic wand (DFD: theoretical ×3, observed 3%).
- [ ] Guard intrinsics with runtime dispatch (`__builtin_cpu_supports`) or
      `#ifdef` fallbacks — this project ships binaries to unknown hardware.
- [ ] **Branchless code is prioritized** where branches are data-dependent
      and avoidable: conditional arithmetic, bitmasks, `count += (x == y)`;
      sort/group data first when it makes branches predictable.
- [ ] `[[likely]]`/`[[unlikely]]` on strongly skewed branches only.
- [ ] `__restrict__` on buffers that don't alias; hoist loop-invariants and
      loop-invariant metadata lookups out of inner loops.
- [ ] `__builtin_prefetch` before long jumps over linked structures — only if
      profiling shows the misses.
- [ ] Manual unrolling rarely wins; let the compiler do it.
- [ ] Benchmark SIMD on real datasets, never synthetic loops.

### Tricks worth trying (small, targeted, easy to revert)

- [ ] **Avoid division** in hot loops: multiply + shift by a precomputed
      reciprocal when the divisor is loop-invariant.
- [ ] **Loop fusion**: combine several passes over the same data into one
      pass — fewer cache misses and less index overhead.
- [ ] **Precomputed lookup tables** (`constexpr`) for common computations
      (log/exp/trig tables, bit-reversal, month lengths) instead of
      recomputing.
- [ ] **Lazy evaluation**: defer expensive work (statistics, materialized
      values, name resolution) until results are actually requested (cf. §3
      lazy statistics, §10 deferred name resolution).
- [ ] **Fast-path / slow-path separation**: handle common data shapes
      (unquoted CSV, no nulls, sorted input) in a lean dedicated path; keep
      the general path correct but cold (cf. §7 parsing fast path).

## 6. Parallelization (precise, per-dataset)

Real results — wildly data-dependent:

| Case | Result | Lesson |
|---|---|---|
| DFD per-column lattices, 4 threads | 3.4× best, slower worst | needs ≥ ~20 columns; tiny tasks → overhead |
| HyUCC Validator, 4 threads | 3.1× / 0.8× | only when validator ≈ 90% of runtime (Amdahl) |
| HyUCC + Sampler, 4 threads | 6.8× / 0.5× | some datasets 30–45% slower |
| ORDER, 8 threads | 1.14× / 1.00× | early exit → nothing to parallelize |
| Faida, 16 threads | 2.6× / slower | mutex contention; lock-free atomics fixed it |

- [ ] Check Amdahl's law first: S = 1 / ((1-p) + p/N). Small parallelizable
      fraction → skip this phase.
- [ ] Profile for the parallelizable fraction, not just hotspots.
- [ ] Granularity: tasks ≥ ~100µs; fuse tiny tasks; coarser beat finer
      "across the board" in HyUCC experiments; use a minimum-work threshold
      before creating tasks.
- [ ] One shared thread pool; never spawn threads per task; make the count
      configurable, default conservative. Work-stealing for uneven
      candidate costs, static scheduling for uniform scans.
- [ ] Per-thread buffers merged at the end > shared mutex-protected
      structures; `thread_local` counters padded against false sharing;
      atomics only for coarse progress reporting.
- [ ] Watch memory bandwidth: partition-intersection workloads are
      bandwidth-bound on many cores — more threads ≠ more speed.
- [ ] NUMA on big servers: first-touch large buffers on the node that uses
      them; avoid cross-socket synchronization in hot paths.
- [ ] Keep output deterministic (sorted merge of per-thread results); handle
      cancellation and worker exceptions without corrupting shared state.
- [ ] Benchmark on many datasets; accept only if no significant regression
      anywhere (project rule).

## 7. I/O and parsing

- [ ] Parsing is excluded from algorithm timing in papers but users feel it:
      measure it separately and fix it if it dominates.
- [ ] Buffered reads / memory-mapped input for read-only files; large read
      buffers; `posix_fadvise` for sequential access.
- [ ] Zero-copy parsing with `std::string_view` over the raw buffer; never
      materialize a `std::string` per cell.
- [ ] `std::from_chars` / fast_float for numbers; never `std::stoi/stod` in
      loops; no regex in hot paths.
- [ ] Fast-path common unquoted/ASCII fields; keep quoted-field and malformed
      handling correct but off the fast path; defer detailed diagnostics
      until an actual error is detected (don't build error messages per
      row).
- [ ] Dictionary-encode / intern strings once at load — algorithms then
      compare integer IDs, not strings (critical for PLI construction).
- [ ] Incremental type inference: eliminate candidate types as rows are
      scanned; stop checking eliminated types; sample-first inference.
- [ ] Avoid per-row virtual dispatch in typed storage; dispatch on column
      type, not cell type.
- [ ] For repeated runs (dynamic validation!), reuse the loaded relation.

## 8. Containers and hashing

Real cases: ORDER — `std::unordered_set` → `boost::unordered_flat_set`
gave up to ×4.95 on one dataset, >1.7× across the board. Faida —
`std::unordered_map` was barely faster than Java (239s vs 238s); switching
to emhash gave ~1.5× everywhere.

- [ ] Profile to find the actual hot container — don't guess.
- [ ] Node-based → open-addressing in hot paths: `ankerl::unordered_dense`,
      `absl::flat_hash_map`, `boost::unordered_flat_map`, `tsl::robin_map`,
      emhash.
- [ ] `std::map`/`std::set` → sorted `std::vector` + binary search or flat
      variants; `std::list` → `std::vector`/`std::deque`.
- [ ] `std::vector<bool>` → explicit bitsets (this project's `Vertical` is
      bitset-based already).
- [ ] Match the container to the access pattern: read-heavy → one hash
      family, write-heavy → another; verify by profiling.
- [ ] Dense integer domains → vector indexed by ID, not a map.
- [ ] Key size matters: composite candidate keys as packed fixed-width
      integers, sorted ID vectors, or bitsets — never concatenated strings
      ("v1|v2|v3") and never a new allocated key object per row.
- [ ] Incremental hashing for candidate extensions; cache prefix hashes for
      attribute combinations; ensure collision handling preserves
      correctness.
- [ ] Benchmark hash-based grouping against sort-and-scan grouping (sort
      often has better locality for large candidates).
- [ ] Reserve hash table capacity before bulk insertion; tune load factor
      only after measurement; reuse tables with `clear()`.
- [ ] Hash quality: verify the hash isn't the bottleneck for bitset keys;
      consider faster hashes (xxHash, wyhash). Ensure the bitset-key hash is
      stable when the bitset is a map key.

## 9. Large data / external memory

When data doesn't fit RAM, performance is a different design problem.

- [ ] Detect when estimated memory usage exceeds a configurable budget;
      provide a memory-budget option.
- [ ] Chunked loading and streaming for algorithms that can operate
      incrementally; two-pass algorithms where they reduce memory pressure.
- [ ] External sorting for sort-based algorithms; spill intermediate
      partitions only when necessary.
- [ ] Sequential disk access, batched writes, compact binary temp formats
      (never serialize pointer-rich in-memory structures); clean up temp
      files safely; make the temp location configurable.
- [ ] Avoid excessive page faults from mapping too much data at once.

## 10. Output and API

- [ ] Do not format results until discovery is complete: store internal
      results as numeric column IDs, resolve names only at serialization.
- [ ] Avoid string concatenation in loops; use buffered output; no flush per
      result.
- [ ] Support result limits / pagination / streaming callbacks for massive
      result sets; ensure result sorting isn't accidentally the dominant
      cost (make deterministic sort optional if not required).
- [ ] Measure serialization (e.g. JSON) separately from discovery.

## 11. Runtime observability (cheap internal counters)

Instrument algorithms with counters to find wasteful work without external
tools — the LLM can add them temporarily for a profiling run:

- [ ] Candidates generated / pruned / validated (search-space efficiency).
- [ ] Partition construction and refinement counts, cache hits vs misses.
- [ ] Allocation count and total bytes in hot phases.
- [ ] Hash-table operations, sort time, per-thread idle time.
- [ ] Keep them cheap (increments, no logging, no formatting); gated behind a
      debug flag so production paths stay clean.

## 12. Build configuration

- [ ] Release build is on (`-O3 -DNDEBUG`, `CMAKE_BUILD_TYPE=Release` —
      verify it's not silently Debug).
- [ ] LTO enabled (`-flto`, thin LTO for faster builds) — measure; LTO +
      `-fno-plt` helps the hot loops.
- [ ] `-march=native` for local deployment builds; portable baseline
      (`-march=x86-64-v3`) for shipped binaries; `-mtune` only where
      portability is guaranteed.
- [ ] Profiling builds: `-g -fno-omit-frame-pointer` (frame pointers are
      required for perf to resolve stacks).
- [ ] **No PGO, BOLT, or any optimization requiring multiple training runs —
      project policy.** (Profiling is allowed; profile-guided *compilation*
      is not.)
- [ ] Compare GCC vs Clang on the workload if a change is borderline.
- [ ] Link an alternative allocator (mimalloc/jemalloc/tcmalloc) only as an
      experiment — lattice-heavy algorithms allocate millions of small
      objects; measure before adopting.
- [ ] Keep exceptions/RTTI as the project uses them; disabling them is a
      project-wide decision, not a local optimization.

## 13. Verification and rules

- [ ] Optimize measured bottlenecks, not guessed ones.
- [ ] Correctness before and after: full test suite, ASan/UBSan/TSan/MSan
      builds stay clean; never benchmark sanitizer builds.
- [ ] Differential testing: compare optimized output against a slower
      reference implementation on small datasets; fuzz parsers and
      candidate generation.
- [ ] Edge cases: empty input, one row, one column, all-null, all-duplicate,
      all-unique, long strings, malformed CSV, Unicode, values at integer
      limits.
- [ ] Exact agreement between sequential and parallel paths; deterministic
      output preserved.
- [ ] Approximate results only as an explicit, configurable mode with
      documented error bounds and seed in result metadata — never silent
      approximation.
- [ ] One controlled change per experiment: profile → hypothesize → smallest
      fix → re-benchmark → correctness → keep only if real.
- [ ] Re-profile after every merged change; performance work is iterative.
- [ ] **Final check of every performance task: merge all new branches back
      into the branch where the user called you, then run all tests plus
      `valgrind` (memcheck), `helgrind`, and `drd`** — the full test suite
      plus memory errors, data races, and deadlocks before reporting done.
      These are the **penultimate and last tasks of the performance task's
      todo file** (`llm/PLAN.md` §2).

## Appendix A. Priority cheat-sheet

1. **Profile** (perf stat/record -F 99 + flamegraph + toplev) → find the real bottleneck class
2. **Fix the algorithm** (pruning, caching, incremental work, sampling) — up to 100×+
3. **Fix copies** (const&, move, reserve, views) — up to 6–50×
4. **Fix containers** (flat hash maps matched to access pattern) — 1.5–6×
5. **Fix memory layout** (contiguous, chunked, arena, CSR clusters) — enables everything below
6. **Parallelize** with correct granularity — 0.5× to 6.8×, data-dependent
7. **SIMD** last, on contiguous data only — typically 1.05–1.3× on top

## Appendix B. Quick per-change review questions

- [ ] Does it add allocation, copies, virtual dispatch, locks, logging, or
      formatting to a hot loop?
- [ ] Does it add work per row or per candidate?
- [ ] Does it increase asymptotic complexity, peak memory, or allocation
      count?
- [ ] Does it introduce a global lock or invalidate caches
      (partition/statistics reuse) too often?
- [ ] Does it preserve pruning correctness, null semantics, exactness
      guarantees, determinism?
- [ ] Was it benchmarked with the PLAN.md §5 protocol, before and after, on
      representative datasets?
