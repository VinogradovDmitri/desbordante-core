# todo_perf_10.md — Performance review checklist: PERFORMANCE.md §10 Output and API

> **Template, not a live todo.** At the start of a **Performance-mode**
> review, copy this file into `bin/`:
>
>     cp llm/todo_perf_10.md bin/todo_perf_10.md
>
> then walk it top to bottom as your own todo file for the §10 phase
> (`llm/AGENT.md` "Review workflow"; `llm/CLAUDE.md` §10 S2;
> `llm/PLAN.md` §2).
>
> - Mark a checkbox `in_progress` before reviewing it, `completed`
>   immediately after (S3); keep the `bin/todo_perf_10.md` file and the
>   in-session todo display in sync (same message).
> - A box is `completed` only after its verification actually ran —
>   command executed / log inspected / result recorded in the session log
>   (S5).
> - Record each box's verdict (**Pass / Fail / N/A / Needs-info**) with
>   `file:line` and *how verified* (build / profiling / inspection only)
>   in the review report (path per `llm/AGENT.md` "Review defaults"), not here.
> - Walk top to bottom, in order; every box gets a verdict (S4).
> - **Delete `bin/todo_perf_10.md` once every checkbox below is
>   `completed`** — a fully checked file is removed after the phase's
>   verification pass (S1; `llm/CLAUDE.md` §0 item 1; `llm/PLAN.md` §2). A
>   leftover completed file is a tracking failure.

Source: `llm/PERFORMANCE.md` §10 "Output and API".
Acceptance criterion: every §10 box has a verdict in the report, this
file is fully checked, and it is deleted.

## Workflow (commit-based — per template)

- [ ] Baseline measurement (quick — not a long benchmark run; record
      numbers in `bin/measurements_<YYYY-MM-DD>.md`)
- [ ] Implementation — walk the §10 checklist below, applying the
      optimizations
- [ ] Measurement after implementation — compare against baseline
      (`llm/PLAN.md` §5 protocol)
- [ ] Create a new commit if there is a measured performance increase;
      if no increase, revert so the working tree returns to baseline
- [ ] Delete this `bin/todo_perf_10.md` file (S1; all checkboxes
      completed)

## §10 — Output and API

- [ ] §10 — No result formatting until discovery is complete: store
      numeric column IDs, resolve names only at serialization
- [ ] §10 — No string concatenation in loops; buffered output; no flush
      per result
- [ ] §10 — Result limits / pagination / streaming for massive result
      sets; make deterministic sort optional if it dominates
- [ ] §10 — Measure serialization (e.g. JSON) separately from discovery
