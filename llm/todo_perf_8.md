# todo_perf_8.md — Performance review checklist: PERFORMANCE.md §8 Containers and hashing

> **Template, not a live todo.** At the start of a **Performance-mode**
> review, copy this file into `bin/`:
>
>     cp llm/todo_perf_8.md bin/todo_perf_8.md
>
> then walk it top to bottom as your own todo file for the §8 phase
> (`llm/AGENT.md` "Review workflow"; `llm/CLAUDE.md` §10 S2;
> `llm/PLAN.md` §2).
>
> - Mark a checkbox `in_progress` before reviewing it, `completed`
>   immediately after (S3); keep the `bin/todo_perf_8.md` file and the
>   in-session todo display in sync (same message).
> - A box is `completed` only after its verification actually ran —
>   command executed / log inspected / result recorded in the session log
>   (S5).
> - Record each box's verdict (**Pass / Fail / N/A / Needs-info**) with
>   `file:line` and *how verified* (build / profiling / inspection only)
>   in the review report (path per `llm/AGENT.md` "Review defaults"), not here.
> - Walk top to bottom, in order; every box gets a verdict (S4).
> - **Delete `bin/todo_perf_8.md` once every checkbox below is
>   `completed`** — a fully checked file is removed after the phase's
>   verification pass (S1; `llm/CLAUDE.md` §0 item 1; `llm/PLAN.md` §2). A
>   leftover completed file is a tracking failure.

Source: `llm/PERFORMANCE.md` §8 "Containers and hashing".
Acceptance criterion: every §8 box has a verdict in the report, this
file is fully checked, and it is deleted.

## Workflow (commit-based — per template)

- [ ] Baseline measurement (quick — not a long benchmark run; record
      numbers in `bin/measurements_<YYYY-MM-DD>.md`)
- [ ] Implementation — walk the §8 checklist below, applying the
      optimizations
- [ ] Measurement after implementation — compare against baseline
      (`llm/PLAN.md` §5 protocol)
- [ ] Create a new commit if there is a measured performance increase;
      if no increase, revert so the working tree returns to baseline
- [ ] Delete this `bin/todo_perf_8.md` file (S1; all checkboxes
      completed)

## §8 — Containers and hashing

- [ ] §8 — Profile to find the actual hot container — don't guess
- [ ] §8 — Node-based → open addressing: `ankerl::unordered_dense`,
      `absl::flat_hash_map`, `boost::unordered_flat_map`,
      `tsl::robin_map`, emhash
- [ ] §8 — `std::map`/`set` → sorted vector + binary search;
      `std::list` → vector/deque; `std::vector<bool>` → explicit
      bitsets (project `Vertical` already is)
- [ ] §8 — Match the container to the access pattern (read- vs
      write-heavy); dense integer domains → vector indexed by ID
- [ ] §8 — Keys: packed fixed-width integers, sorted ID vectors, or
      bitsets — never concatenated strings ("v1|v2|v3"), never a new
      allocated key object per row
- [ ] §8 — Incremental hashing for candidate extensions; cache prefix
      hashes; reserve before bulk insert; reuse with `clear()`; tune
      load factor only after measurement
- [ ] §8 — Benchmark hash-based grouping vs sort-and-scan (sort often
      has better locality for large candidates)
- [ ] §8 — Hash quality: xxHash/wyhash if the hash is the bottleneck;
      stable bitset-key hash when a bitset is a map key
