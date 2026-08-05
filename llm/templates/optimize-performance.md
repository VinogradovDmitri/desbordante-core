# Optimize: `<algorithm>` — `<what is slow>`

> Fill-in template for performance work. Copy this file, replace every
> `<placeholder>`, delete guidance lines as you go. Checklist:
> `llm/PERFORMANCE.md`. Measurement protocol: `llm/PLAN.md` §5. Branch rules:
> `llm/PLAN.md` §3; joint validation: `llm/PLAN.md` §4.

## 1. Baseline (before any change)

- Bottleneck classification: `<CPU / memory-bandwidth / cache-latency /
  allocation / I/O / lock / branch-prediction>` — with the profiling evidence
- Baseline measurement (full `llm/PLAN.md` §5 protocol state):
  `<command>` → `<numbers>`
- Branch: `perf/<todo-num>-<short-slug>` — one branch changes **one** type of
  thing (one fast path, one threading change, one data structure)
- Expected improvement (numeric, recorded **before** implementation):
  `<e.g. ≥ 5% faster on examples/datasets/<file>.csv under the §5 protocol>`
- Format to match (past example): `<an existing optimized algorithm /
  structure to mirror>`

## 2. Acceptance criteria (precise, checkable)

- [ ] Expected improvement measured under the same §5 protocol state → `<target>`
- [ ] No correctness change: `ctest --test-dir build -R "<algo-regex>"` passes
- [ ] Examples + snapshots pass **unchanged** (output must not move)
- [ ] Results identical on `<datasets>` before/after

## 3. Steps (in order)

- [ ] Profile first (`llm/PERFORMANCE.md` §1); record numbers + machine state
      in `bin/measurements_<YYYY-MM-DD>.md`
- [ ] Change one thing (`llm/PLAN.md` §3)
- [ ] Re-measure under the same protocol state; compare against baseline
- [ ] Keep only if it pays (measured gain, or §4(b) redeeming quality with no
      current-processor regression); revert otherwise
- [ ] Joint validation if another perf branch is pending (`llm/PLAN.md` §4)
- [ ] Verification chain (`llm/DEVELOPMENT.md` §6) with the conditional loop
- [ ] Second-pass review of both the diff and the numbers (fresh pass or
      second model) → `<findings fixed / explicitly waived>`

## 4. Report

Base vs branch numbers, machine state (governor, pinned frequency, isolated
cores), and the keep/revert decision with its reason — recorded in
`bin/measurements_<YYYY-MM-DD>.md` and stated in the final summary.
