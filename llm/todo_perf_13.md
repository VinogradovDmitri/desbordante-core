# todo_perf_13.md — Performance review checklist: PERFORMANCE.md §13 Verification and rules

> **Template, not a live todo.** At the start of a **Performance-mode**
> review, copy this file into `bin/`:
>
>     cp llm/todo_perf_13.md bin/todo_perf_13.md
>
> then walk it top to bottom as your own todo file for the §13 phase
> (`llm/AGENT.md` "Review workflow"; `llm/CLAUDE.md` §10 S2;
> `llm/PLAN.md` §2).
>
> - Mark a checkbox `in_progress` before reviewing it, `completed`
>   immediately after (S3); keep the `bin/todo_perf_13.md` file and the
>   in-session todo display in sync (same message).
> - A box is `completed` only after its verification actually ran —
>   command executed / log inspected / result recorded in the session log
>   (S5).
> - Record each box's verdict (**Pass / Fail / N/A / Needs-info**) with
>   `file:line` and *how verified* (build / profiling / inspection only)
>   in the review report (path per `llm/AGENT.md` "Review defaults"), not here.
> - Walk top to bottom, in order; every box gets a verdict (S4).
> - **Delete `bin/todo_perf_13.md` once every checkbox below is
>   `completed`** — a fully checked file is removed after the phase's
>   verification pass (S1; `llm/CLAUDE.md` §0 item 1; `llm/PLAN.md` §2). A
>   leftover completed file is a tracking failure.

Source: `llm/PERFORMANCE.md` §13 "Verification and rules".
Acceptance criterion: every §13 box has a verdict in the report, this
file is fully checked, and it is deleted.

## Workflow (commit-based — per template)

- [ ] Baseline measurement (quick — not a long benchmark run; record
      numbers in `bin/measurements_<YYYY-MM-DD>.md`)
- [ ] Implementation — walk the §13 checklist below, applying the
      optimizations
- [ ] Measurement after implementation — compare against baseline
      (`llm/PLAN.md` §5 protocol)
- [ ] Create a new commit if there is a measured performance increase;
      if no increase, revert so the working tree returns to baseline
- [ ] Delete this `bin/todo_perf_13.md` file (S1; all checkboxes
      completed)

## §13 — Verification and rules

- [ ] §13 — Optimize measured bottlenecks, not guessed ones
- [ ] §13 — Correctness before and after: the algorithm's targeted tests
      (`ctest --test-dir build -R "<algo>"`, `llm/DEVELOPMENT.md` §2);
      ASan/UBSan are CI-only — never benchmark them; no sanitizer builds
      locally
- [ ] §13 — Differential testing vs a slower reference on small
      datasets; fuzz parsers and candidate generation
- [ ] §13 — Edge cases: empty, one row/column, all-null/duplicate/
      unique, long strings, malformed CSV, Unicode, integer limits
- [ ] §13 — Exact agreement between sequential and parallel paths;
      deterministic output preserved
- [ ] §13 — Approximation only as an explicit, configurable mode with
      documented error bounds and seed in result metadata — never
      silent
- [ ] §13 — One controlled change per experiment: profile →
      hypothesize → smallest fix → re-benchmark → correctness → keep
      only if real
- [ ] §13 — Re-profile after every merged change; performance work is
      iterative
- [ ] §13 — **Final check of every perf task: per `llm/PLAN.md` §2 —
      merge all new branches back into the branch where the user called
      you, then run the **targeted tests** (`ctest --test-dir build -R
      "<algo>"`; `llm/DEVELOPMENT.md` §2 — never the full suite) plus
      `valgrind` (memcheck), `helgrind`, and `drd`** — memory errors,
      data races, deadlocks. These are the penultimate and last tasks
      of the perf todo file
