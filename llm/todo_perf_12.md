# todo_perf_12.md — Performance review checklist: PERFORMANCE.md §12 Build configuration

> **Template, not a live todo.** At the start of a **Performance-mode**
> review, copy this file into `bin/`:
>
>     cp llm/todo_perf_12.md bin/todo_perf_12.md
>
> then walk it top to bottom as your own todo file for the §12 phase
> (`llm/AGENT.md` "Review workflow"; `llm/CLAUDE.md` §10 S2;
> `llm/PLAN.md` §2).
>
> - Mark a checkbox `in_progress` before reviewing it, `completed`
>   immediately after (S3); keep the `bin/todo_perf_12.md` file and the
>   in-session todo display in sync (same message).
> - A box is `completed` only after its verification actually ran —
>   command executed / log inspected / result recorded in the session log
>   (S5).
> - Record each box's verdict (**Pass / Fail / N/A / Needs-info**) with
>   `file:line` and *how verified* (build / profiling / inspection only)
>   in the review report (path per `llm/AGENT.md` "Review defaults"), not here.
> - Walk top to bottom, in order; every box gets a verdict (S4).
> - **Delete `bin/todo_perf_12.md` once every checkbox below is
>   `completed`** — a fully checked file is removed after the phase's
>   verification pass (S1; `llm/CLAUDE.md` §0 item 1; `llm/PLAN.md` §2). A
>   leftover completed file is a tracking failure.

Source: `llm/PERFORMANCE.md` §12 "Build configuration".
Acceptance criterion: every §12 box has a verdict in the report, this
file is fully checked, and it is deleted.

## Workflow (commit-based — per template)

- [ ] Baseline measurement (quick — not a long benchmark run; record
      numbers in `bin/measurements_<YYYY-MM-DD>.md`)
- [ ] Implementation — walk the §12 checklist below, applying the
      optimizations
- [ ] Measurement after implementation — compare against baseline
      (`llm/PLAN.md` §5 protocol)
- [ ] Create a new commit if there is a measured performance increase;
      if no increase, revert so the working tree returns to baseline
- [ ] Delete this `bin/todo_perf_12.md` file (S1; all checkboxes
      completed)

## §12 — Build configuration

- [ ] §12 — Release on (`-O3 -DNDEBUG`, `CMAKE_BUILD_TYPE=Release` —
      verify not silently Debug); LTO (`-flto`, thin for faster builds)
      — measure; `-fno-plt` helps hot loops
- [ ] §12 — `-march=native` for local builds; portable baseline
      (`-march=x86-64-v3`) for shipped binaries; `-mtune` only where
      portability is guaranteed
- [ ] §12 — Profiling builds: `-g -fno-omit-frame-pointer` (needed for
      perf stacks)
- [ ] §12 — **No PGO, BOLT, or any multi-training-run optimization —
      project policy** (profiling allowed; profile-guided
      *compilation* is not)
- [ ] §12 — Compare GCC vs Clang on the workload if a change is
      borderline; alternative allocator only as an experiment; keep
      exceptions/RTTI as the project uses them
