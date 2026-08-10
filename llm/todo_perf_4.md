# todo_perf_4.md — Performance review checklist: PERFORMANCE.md §4 Data structures, memory layout, allocations

> **Template, not a live todo.** At the start of a **Performance-mode**
> review, copy this file into `bin/`:
>
>     cp llm/todo_perf_4.md bin/todo_perf_4.md
>
> then walk it top to bottom as your own todo file for the §4 phase
> (`llm/AGENT.md` "Review workflow"; `llm/CLAUDE.md` §10 S2;
> `llm/PLAN.md` §2).
>
> - Mark a checkbox `in_progress` before reviewing it, `completed`
>   immediately after (S3); keep the `bin/todo_perf_4.md` file and the
>   in-session todo display in sync (same message).
> - A box is `completed` only after its verification actually ran —
>   command executed / log inspected / result recorded in the session log
>   (S5).
> - Record each box's verdict (**Pass / Fail / N/A / Needs-info**) with
>   `file:line` and *how verified* (build / profiling / inspection only)
>   in the review report (path per `llm/AGENT.md` "Review defaults"), not here.
> - Walk top to bottom, in order; every box gets a verdict (S4).
> - **Delete `bin/todo_perf_4.md` once every checkbox below is
>   `completed`** — a fully checked file is removed after the phase's
>   verification pass (S1; `llm/CLAUDE.md` §0 item 1; `llm/PLAN.md` §2). A
>   leftover completed file is a tracking failure.

Source: `llm/PERFORMANCE.md` §4 "Data structures, memory layout,
allocations".
Acceptance criterion: every §4 box has a verdict in the report, this
file is fully checked, and it is deleted.

## Workflow (commit-based — per template)

- [ ] Baseline measurement (quick — not a long benchmark run; record
      numbers in `bin/measurements_<YYYY-MM-DD>.md`)
- [ ] Implementation — walk the §4 checklist below, applying the
      optimizations
- [ ] Measurement after implementation — compare against baseline
      (`llm/PLAN.md` §5 protocol)
- [ ] Create a new commit if there is a measured performance increase;
      if no increase, revert so the working tree returns to baseline
- [ ] Delete this `bin/todo_perf_4.md` file (S1; all checkboxes
      completed)

## §4 — Data structures, memory layout, allocations

- [ ] §4 — Flatten pointer-chasing structures into contiguous arrays;
      arena/pool-allocate hot objects (lattice vertices, clusters, trie
      nodes)
- [ ] §4 — Many small bitsets in one flat buffer + index, not N heap
      allocations; track lengths to skip unused trailing words
- [ ] §4 — SoA over AoS (this is a columnar project); PLI clusters:
      single flat `std::vector<int>` + offsets (CSR) vs
      deque-of-vectors — evaluate for your access pattern
- [ ] §4 — 64-byte alignment for hot buffers; `alignas(64)` per-thread
      counters (false sharing); compact flags arrays instead of
      scattered `bool`s
- [ ] §4 — `reserve()` when size is known (not premature);
      `emplace_back`; `clear()` + reuse instead of reconstructing
- [ ] §4 — Arena/scratch buffers for per-iteration temporaries; reset
      between phases
- [ ] §4 — Smallest safe integer types (`uint32_t`/`uint16_t`);
      missing-value sentinel outside the valid dictionary-ID range
- [ ] §4 — **Cache layer** where profiling shows repeated identical work
      — small reuse-likelihood/LRU (intersections, error metrics,
      cardinality, statistics); measure hit rate, keep only if it
      pays
- [ ] §4 — Huge pages (`madvise(MADV_HUGEPAGE)`) on multi-GB structures
      — benchmark before adopting
- [ ] §4 — Try jemalloc/mimalloc/tcmalloc in the benchmark build only;
      revert if no gain
- [ ] §4 — Indexes: permutation/rank arrays over values — sort row IDs,
      not rows; avoid materializing copies
- [ ] §4 — Cache-friendly chunking (L2/L3 blocks, inner loop over the
      whole chunk); hot/cold field splitting
- [ ] §4 — Roaring/compressed bitmaps for sparse large row sets if deps
      permit — benchmark first
- [ ] §4 — Check allocation count/page faults before/after — cheap win
