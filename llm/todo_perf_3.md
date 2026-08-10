# todo_perf_3.md — Performance review checklist: PERFORMANCE.md §3 Per-problem specifics

> **Template, not a live todo.** At the start of a **Performance-mode**
> review, copy this file into `bin/`:
>
>     cp llm/todo_perf_3.md bin/todo_perf_3.md
>
> then walk it top to bottom as your own todo file for the §3 phase
> (`llm/AGENT.md` "Review workflow"; `llm/CLAUDE.md` §10 S2;
> `llm/PLAN.md` §2).
>
> - Mark a checkbox `in_progress` before reviewing it, `completed`
>   immediately after (S3); keep the `bin/todo_perf_3.md` file and the
>   in-session todo display in sync (same message).
> - A box is `completed` only after its verification actually ran —
>   command executed / log inspected / result recorded in the session log
>   (S5).
> - Record each box's verdict (**Pass / Fail / N/A / Needs-info**) with
>   `file:line` and *how verified* (build / profiling / inspection only)
>   in the review report (path per `llm/AGENT.md` "Review defaults"), not here.
> - Walk top to bottom, in order; every box gets a verdict (S4).
> - **Delete `bin/todo_perf_3.md` once every checkbox below is
>   `completed`** — a fully checked file is removed after the phase's
>   verification pass (S1; `llm/CLAUDE.md` §0 item 1; `llm/PLAN.md` §2). A
>   leftover completed file is a tracking failure.

Source: `llm/PERFORMANCE.md` §3 "Per-problem specifics".
Note: §3 is **prose guidance** (no checkboxes in PERFORMANCE.md) —
per-problem specifics for FD/AFD, UCC, IND, OD/sorting, Statistics. Read
the full section and confirm the guidance relevant to the reviewed
problem type is applied/considered; cite `PERFORMANCE.md:line` in the
report for any deviation.
Acceptance criterion: the §3 confirm box is checked, a verdict is
recorded in the report, and this file is deleted.

## Workflow (commit-based — per template)

- [ ] Baseline measurement (quick — not a long benchmark run; record
      numbers in `bin/measurements_<YYYY-MM-DD>.md`)
- [ ] Implementation — walk the §3 checklist below, applying the
      optimizations
- [ ] Measurement after implementation — compare against baseline
      (`llm/PLAN.md` §5 protocol)
- [ ] Create a new commit if there is a measured performance increase;
      if no increase, revert so the working tree returns to baseline
- [ ] Delete this `bin/todo_perf_3.md` file (S1; all checkboxes
      completed)

## §3 — Per-problem specifics (prose — read and confirm)

- [ ] §3 — Read the full §3 prose and confirm the guidance for the
      reviewed problem type(s) (FD/AFD, UCC, IND, OD/sorting,
      Statistics) is applied where applicable; note any deviation with
      `PERFORMANCE.md:line` in the report
