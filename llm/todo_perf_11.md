# todo_perf_11.md — Performance review checklist: PERFORMANCE.md §11 Runtime observability (cheap internal counters)

> **Template, not a live todo.** At the start of a **Performance-mode**
> review, copy this file into `bin/`:
>
>     cp llm/todo_perf_11.md bin/todo_perf_11.md
>
> then walk it top to bottom as your own todo file for the §11 phase
> (`llm/AGENT.md` "Review workflow"; `llm/CLAUDE.md` §10 S2;
> `llm/PLAN.md` §2).
>
> - Mark a checkbox `in_progress` before reviewing it, `completed`
>   immediately after (S3); keep the `bin/todo_perf_11.md` file and the
>   in-session todo display in sync (same message).
> - A box is `completed` only after its verification actually ran —
>   command executed / log inspected / result recorded in the session log
>   (S5).
> - Record each box's verdict (**Pass / Fail / N/A / Needs-info**) with
>   `file:line` and *how verified* (build / profiling / inspection only)
>   in the review report (path per `llm/AGENT.md` "Review defaults"), not here.
> - Walk top to bottom, in order; every box gets a verdict (S4).
> - **Delete `bin/todo_perf_11.md` once every checkbox below is
>   `completed`** — a fully checked file is removed after the phase's
>   verification pass (S1; `llm/CLAUDE.md` §0 item 1; `llm/PLAN.md` §2). A
>   leftover completed file is a tracking failure.

Source: `llm/PERFORMANCE.md` §11 "Runtime observability (cheap internal
counters)".
Note: §11 is **prose guidance** (no checkboxes in PERFORMANCE.md) —
temporary profiling counters (candidates generated/pruned/validated,
partition construction/refinement counts, cache hits vs misses,
allocation count/bytes, hash-table ops, sort time, per-thread idle);
keep them cheap (increments, no logging/formatting); gate behind a
debug flag. Read the full section and confirm the guidance is
considered where applicable; cite `PERFORMANCE.md:line` in the report
for any deviation.
Acceptance criterion: the §11 confirm box is checked, a verdict is
recorded in the report, and this file is deleted.

## Workflow (commit-based — per template)

- [ ] Baseline measurement (quick — not a long benchmark run; record
      numbers in `bin/measurements_<YYYY-MM-DD>.md`)
- [ ] Implementation — walk the §11 checklist below, applying the
      optimizations
- [ ] Measurement after implementation — compare against baseline
      (`llm/PLAN.md` §5 protocol)
- [ ] Create a new commit if there is a measured performance increase;
      if no increase, revert so the working tree returns to baseline
- [ ] Delete this `bin/todo_perf_11.md` file (S1; all checkboxes
      completed)

## §11 — Runtime observability (prose — read and confirm)

- [ ] §11 — Read the full §11 prose and confirm the runtime-observability
      guidance (cheap internal counters, gated behind a debug flag) is
      applied/considered where applicable; note any deviation with
      `PERFORMANCE.md:line` in the report
