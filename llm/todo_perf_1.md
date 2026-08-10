# todo_perf_1.md — Performance review checklist: PERFORMANCE.md §1 Measurement foundation (first, non-negotiable)

> **Template, not a live todo.** At the start of a **Performance-mode**
> review, copy this file into `bin/`:
>
>     cp llm/todo_perf_1.md bin/todo_perf_1.md
>
> then walk it top to bottom as your own todo file for the §1 phase
> (`llm/AGENT.md` "Review workflow"; `llm/CLAUDE.md` §10 S2;
> `llm/PLAN.md` §2).
>
> - Mark a checkbox `in_progress` before reviewing it, `completed`
>   immediately after (S3); keep the `bin/todo_perf_1.md` file and the
>   in-session todo display in sync (same message).
> - A box is `completed` only after its verification actually ran —
>   command executed / log inspected / result recorded in the session log
>   (S5).
> - Record each box's verdict (**Pass / Fail / N/A / Needs-info**) with
>   `file:line` and *how verified* (build / profiling / inspection only)
>   in the review report (path per `llm/AGENT.md` "Review defaults"), not here.
> - Walk top to bottom, in order; every box gets a verdict (S4).
> - **Delete `bin/todo_perf_1.md` once every checkbox below is
>   `completed`** — a fully checked file is removed after the phase's
>   verification pass (S1; `llm/CLAUDE.md` §0 item 1; `llm/PLAN.md` §2). A
>   leftover completed file is a tracking failure.

Source: `llm/PERFORMANCE.md` §1 "Measurement foundation (first,
non-negotiable)".
Acceptance criterion: every §1 box has a verdict in the report, this
file is fully checked, and it is deleted.

## Workflow (commit-based — per template)

- [ ] Baseline measurement (quick — not a long benchmark run; record
      numbers in `bin/measurements_<YYYY-MM-DD>.md`)
- [ ] Implementation — walk the §1 checklist below, applying the
      optimizations
- [ ] Measurement after implementation — compare against baseline
      (`llm/PLAN.md` §5 protocol)
- [ ] Create a new commit if there is a measured performance increase;
      if no increase, revert so the working tree returns to baseline
- [ ] Delete this `bin/todo_perf_1.md` file (S1; all checkboxes
      completed)

## §1 — Measurement foundation (first, non-negotiable)

- [ ] §1 — Baseline benchmark suite: fixed datasets (small/large,
      sparse/dense, few/many columns, high/low cardinality,
      unique/duplicate/null/string-heavy, narrow/wide); fixed random
      seeds; immutable inputs
- [ ] §1 — Define performance targets before optimizing (max wall time,
      peak memory, throughput rows/s and cells/s, scaling)
- [ ] §1 — Measurement protocol: `llm/PLAN.md` §5 (pinned frequency,
      isolated cores, caches dropped, `perf stat -r 10`, median). Never
      compare runs from different machine states
- [ ] §1 — Separate parsing vs algorithm timing; cold-cache vs
      warm-cache runs
- [ ] §1 — Measure memory too: peak RSS, allocation count/bytes
      (heaptrack/massif)
- [ ] §1 — Record every experiment (machine, compiler, flags, dataset,
      before/after, date) in `bin/measurements_<YYYY-MM-DD>.md` with the
      machine state
- [ ] §1 — Profile build: Release `./build.sh -p -b -j4 -f` — no debug
      symbols, no sanitizers (both consume memory and slow things);
      never benchmark sanitizer builds. `-j` = CNT_CPU_CORE / 2 computed
      on the user's PC
- [ ] §1 — Verify correctness before and after every change
