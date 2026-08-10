# todo_perf_5.md — Performance review checklist: PERFORMANCE.md §5 Microarchitectural and SIMD

> **Template, not a live todo.** At the start of a **Performance-mode**
> review, copy this file into `bin/`:
>
>     cp llm/todo_perf_5.md bin/todo_perf_5.md
>
> then walk it top to bottom as your own todo file for the §5 phase
> (`llm/AGENT.md` "Review workflow"; `llm/CLAUDE.md` §10 S2;
> `llm/PLAN.md` §2).
>
> - Mark a checkbox `in_progress` before reviewing it, `completed`
>   immediately after (S3); keep the `bin/todo_perf_5.md` file and the
>   in-session todo display in sync (same message).
> - A box is `completed` only after its verification actually ran —
>   command executed / log inspected / result recorded in the session log
>   (S5).
> - Record each box's verdict (**Pass / Fail / N/A / Needs-info**) with
>   `file:line` and *how verified* (build / profiling / inspection only)
>   in the review report (path per `llm/AGENT.md` "Review defaults"), not here.
> - Walk top to bottom, in order; every box gets a verdict (S4).
> - **Delete `bin/todo_perf_5.md` once every checkbox below is
>   `completed`** — a fully checked file is removed after the phase's
>   verification pass (S1; `llm/CLAUDE.md` §0 item 1; `llm/PLAN.md` §2). A
>   leftover completed file is a tracking failure.

Source: `llm/PERFORMANCE.md` §5 "Microarchitectural and SIMD".
Acceptance criterion: every §5 box has a verdict in the report, this
file is fully checked, and it is deleted.

## Workflow (commit-based — per template)

- [ ] Baseline measurement (quick — not a long benchmark run; record
      numbers in `bin/measurements_<YYYY-MM-DD>.md`)
- [ ] Implementation — walk the §5 checklist below, applying the
      optimizations
- [ ] Measurement after implementation — compare against baseline
      (`llm/PLAN.md` §5 protocol)
- [ ] Create a new commit if there is a measured performance increase;
      if no increase, revert so the working tree returns to baseline
- [ ] Delete this `bin/todo_perf_5.md` file (S1; all checkboxes
      completed)

## §5 — Microarchitectural and SIMD

- [ ] §5 — Auto-vectorization first: simple loops, contiguous access,
      no virtual calls; verify with `-fopt-info-vec` / `-Rpass=vector`
- [ ] §5 — **Bit manipulation is prioritized over SIMD**: word-level
      tricks on `uint64_t` — subset check `(a & b) == a`,
      intersection/union/difference via `& | ^`,
      `std::popcount`/ctz/rotl builtins, bit-packing to halve memory.
      Portable; intrinsics only when these aren't enough
- [ ] §5 — SIMD only after layout is fixed, on contiguous buffers (a
      magnifier, not a magic wand); runtime dispatch
      (`__builtin_cpu_supports`) or `#ifdef` fallbacks — binaries ship
      to unknown hardware; benchmark on real datasets, never synthetic
      loops
- [ ] §5 — **Branchless code is prioritized**: conditional arithmetic,
      bitmasks, `count += (x == y)`; sort/group first to make branches
      predictable
- [ ] §5 — `[[likely]]`/`[[unlikely]]` on strongly skewed branches only;
      `__restrict__` on non-aliasing buffers; hoist loop invariants
- [ ] §5 — `__builtin_prefetch` only if profiling shows the misses; no
      manual unrolling — let the compiler do it
