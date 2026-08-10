# todo_perf_7.md — Performance review checklist: PERFORMANCE.md §7 I/O and parsing

> **Template, not a live todo.** At the start of a **Performance-mode**
> review, copy this file into `bin/`:
>
>     cp llm/todo_perf_7.md bin/todo_perf_7.md
>
> then walk it top to bottom as your own todo file for the §7 phase
> (`llm/AGENT.md` "Review workflow"; `llm/CLAUDE.md` §10 S2;
> `llm/PLAN.md` §2).
>
> - Mark a checkbox `in_progress` before reviewing it, `completed`
>   immediately after (S3); keep the `bin/todo_perf_7.md` file and the
>   in-session todo display in sync (same message).
> - A box is `completed` only after its verification actually ran —
>   command executed / log inspected / result recorded in the session log
>   (S5).
> - Record each box's verdict (**Pass / Fail / N/A / Needs-info**) with
>   `file:line` and *how verified* (build / profiling / inspection only)
>   in the review report (path per `llm/AGENT.md` "Review defaults"), not here.
> - Walk top to bottom, in order; every box gets a verdict (S4).
> - **Delete `bin/todo_perf_7.md` once every checkbox below is
>   `completed`** — a fully checked file is removed after the phase's
>   verification pass (S1; `llm/CLAUDE.md` §0 item 1; `llm/PLAN.md` §2). A
>   leftover completed file is a tracking failure.

Source: `llm/PERFORMANCE.md` §7 "I/O and parsing".
Acceptance criterion: every §7 box has a verdict in the report, this
file is fully checked, and it is deleted.

## Workflow (commit-based — per template)

- [ ] Baseline measurement (quick — not a long benchmark run; record
      numbers in `bin/measurements_<YYYY-MM-DD>.md`)
- [ ] Implementation — walk the §7 checklist below, applying the
      optimizations
- [ ] Measurement after implementation — compare against baseline
      (`llm/PLAN.md` §5 protocol)
- [ ] Create a new commit if there is a measured performance increase;
      if no increase, revert so the working tree returns to baseline
- [ ] Delete this `bin/todo_perf_7.md` file (S1; all checkboxes
      completed)

## §7 — I/O and parsing

- [ ] §7 — Measure parsing separately; fix it if it dominates
      user-visible time
- [ ] §7 — Buffered reads / memory-mapped input; large buffers;
      `posix_fadvise` for sequential access
- [ ] §7 — Zero-copy `std::string_view` over the raw buffer; never a
      `std::string` per cell
- [ ] §7 — `std::from_chars` / fast_float; never `stoi/stod` in loops;
      no regex in hot paths
- [ ] §7 — Fast path for common unquoted/ASCII fields; quoted/malformed
      handling correct but cold; defer diagnostics until an actual
      error
- [ ] §7 — Dictionary-encode / intern strings once at load — algorithms
      compare integer IDs (critical for PLI construction)
- [ ] §7 — Incremental type inference: eliminate candidates as rows are
      scanned; dispatch on column type, not cell type
- [ ] §7 — For repeated runs (dynamic validation): reuse the loaded
      relation
