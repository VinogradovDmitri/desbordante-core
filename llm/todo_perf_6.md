# todo_perf_6.md — Performance review checklist: PERFORMANCE.md §6 Parallelization (precise, per-dataset)

> **Template, not a live todo.** At the start of a **Performance-mode**
> review, copy this file into `bin/`:
>
>     cp llm/todo_perf_6.md bin/todo_perf_6.md
>
> then walk it top to bottom as your own todo file for the §6 phase
> (`llm/AGENT.md` "Review workflow"; `llm/CLAUDE.md` §10 S2;
> `llm/PLAN.md` §2).
>
> - Mark a checkbox `in_progress` before reviewing it, `completed`
>   immediately after (S3); keep the `bin/todo_perf_6.md` file and the
>   in-session todo display in sync (same message).
> - A box is `completed` only after its verification actually ran —
>   command executed / log inspected / result recorded in the session log
>   (S5).
> - Record each box's verdict (**Pass / Fail / N/A / Needs-info**) with
>   `file:line` and *how verified* (build / profiling / inspection only)
>   in the review report (path per `llm/AGENT.md` "Review defaults"), not here.
> - Walk top to bottom, in order; every box gets a verdict (S4).
> - **Delete `bin/todo_perf_6.md` once every checkbox below is
>   `completed`** — a fully checked file is removed after the phase's
>   verification pass (S1; `llm/CLAUDE.md` §0 item 1; `llm/PLAN.md` §2). A
>   leftover completed file is a tracking failure.

Source: `llm/PERFORMANCE.md` §6 "Parallelization (precise, per-dataset)".
Acceptance criterion: every §6 box has a verdict in the report, this
file is fully checked, and it is deleted.

## Workflow (commit-based — per template)

- [ ] Baseline measurement (quick — not a long benchmark run; record
      numbers in `bin/measurements_<YYYY-MM-DD>.md`)
- [ ] Implementation — walk the §6 checklist below, applying the
      optimizations
- [ ] Measurement after implementation — compare against baseline
      (`llm/PLAN.md` §5 protocol)
- [ ] Create a new commit if there is a measured performance increase;
      if no increase, revert so the working tree returns to baseline
- [ ] Delete this `bin/todo_perf_6.md` file (S1; all checkboxes
      completed)

## §6 — Parallelization (precise, per-dataset)

- [ ] §6 — Amdahl first: S = 1/((1-p) + p/N); small parallelizable
      fraction → skip this phase
- [ ] §6 — Profile for the parallelizable fraction, not just hotspots;
      granularity: tasks ≥ ~100µs; fuse tiny tasks; minimum-work
      threshold
- [ ] §6 — One shared thread pool, never per-task threads; configurable
      count, default conservative; work-stealing for uneven costs,
      static for uniform scans
- [ ] §6 — Per-thread buffers merged at the end > mutex-protected shared
      structures; `thread_local` counters padded; atomics only for
      coarse progress
- [ ] §6 — Watch memory bandwidth — partition intersections saturate
      on many cores; NUMA: first-touch large buffers on the using node
- [ ] §6 — Deterministic output (sorted merge of per-thread results);
      handle cancellation/exceptions without corrupting shared state
- [ ] §6 — Accept only if no significant regression anywhere on many
      datasets (project rule)
