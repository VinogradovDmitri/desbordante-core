# todo_perf_9.md — Performance review checklist: PERFORMANCE.md §9 Large data / external memory

> **Template, not a live todo.** At the start of a **Performance-mode**
> review, copy this file into `bin/`:
>
>     cp llm/todo_perf_9.md bin/todo_perf_9.md
>
> then walk it top to bottom as your own todo file for the §9 phase
> (`llm/AGENT.md` "Review workflow"; `llm/CLAUDE.md` §10 S2;
> `llm/PLAN.md` §2).
>
> - Mark a checkbox `in_progress` before reviewing it, `completed`
>   immediately after (S3); keep the `bin/todo_perf_9.md` file and the
>   in-session todo display in sync (same message).
> - A box is `completed` only after its verification actually ran —
>   command executed / log inspected / result recorded in the session log
>   (S5).
> - Record each box's verdict (**Pass / Fail / N/A / Needs-info**) with
>   `file:line` and *how verified* (build / profiling / inspection only)
>   in the review report (path per `llm/AGENT.md` "Review defaults"), not here.
> - Walk top to bottom, in order; every box gets a verdict (S4).
> - **Delete `bin/todo_perf_9.md` once every checkbox below is
>   `completed`** — a fully checked file is removed after the phase's
>   verification pass (S1; `llm/CLAUDE.md` §0 item 1; `llm/PLAN.md` §2). A
>   leftover completed file is a tracking failure.

Source: `llm/PERFORMANCE.md` §9 "Large data / external memory".
Acceptance criterion: every §9 box has a verdict in the report, this
file is fully checked, and it is deleted.

## Workflow (commit-based — per template)

- [ ] Baseline measurement (quick — not a long benchmark run; record
      numbers in `bin/measurements_<YYYY-MM-DD>.md`)
- [ ] Implementation — walk the §9 checklist below, applying the
      optimizations
- [ ] Measurement after implementation — compare against baseline
      (`llm/PLAN.md` §5 protocol)
- [ ] Create a new commit if there is a measured performance increase;
      if no increase, revert so the working tree returns to baseline
- [ ] Delete this `bin/todo_perf_9.md` file (S1; all checkboxes
      completed)

## §9 — Large data / external memory

- [ ] §9 — Detect estimated usage over a configurable budget;
      memory-budget option; chunked/streaming for incremental
      algorithms; two-pass where it reduces pressure
- [ ] §9 — External sorting; spill intermediate partitions only when
      necessary
- [ ] §9 — Sequential disk access, batched writes, compact binary temp
      formats (never serialize pointer-rich in-memory structures);
      configurable temp location; clean up temp files safely
- [ ] §9 — Avoid excessive page faults from mapping too much at once
