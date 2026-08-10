# todo_rules_18.md — Design review checklist: RULES.md §18 Development philosophy

> **Template, not a live todo.** At the start of a **Design-mode** review,
> copy this file into `bin/`:
>
>     cp llm/todo_rules_18.md bin/todo_rules_18.md
>
> then walk it top to bottom as your own todo file for the §18 phase
> (`llm/AGENT.md` "Review workflow"; `llm/CLAUDE.md` §10 S2; `llm/PLAN.md`
> §2).
>
> - Mark a checkbox `in_progress` before reviewing it, `completed`
>   immediately after (S3); keep the `bin/todo_rules_18.md` file and the
>   in-session todo display in sync (same message).
> - A box is `completed` only after its verification actually ran —
>   command executed / log inspected / result recorded in the session log
>   (S5).
> - Record each box's verdict (**Pass / Fail / N/A / Needs-info**) with
>   `file:line` and *how verified* (build / profiling / inspection only)
>   in the review report (path per `llm/AGENT.md` "Review defaults"), not here.
> - Walk top to bottom, in order; every box gets a verdict (S4).
> - **Delete `bin/todo_rules_18.md` once every checkbox below is
>   `completed`** — a fully checked file is removed after the phase's
>   verification pass (S1; `llm/CLAUDE.md` §0 item 1; `llm/PLAN.md` §2). A
>   leftover completed file is a tracking failure.

Source: `llm/RULES.md` §18 "Development philosophy".
Acceptance criterion: every §18 box has a verdict in the report, this
file is fully checked, and it is deleted.

## §18 — Development philosophy

- [ ] §18 — Implements only the efficiently and unambiguously
      implementable "core"; NP-hard/approximate work pushed to Python
      (e.g. DC minimal deletion set)
- [ ] §18 — No over-engineering (e.g. no explicit column selection for
      FD); small high-value low-cost additions acceptable (e.g.
      header-row handling)
- [ ] §18 — No standalone theoretical features
      (consistency/derivability/triviality/minimality) beyond what a
      mining/validation algorithm needs
- [ ] §18 — Data structures duplicated rather than shared; no mutation of
      a shared structure for one algorithm's benefit
- [ ] §18 — `Execute` does not corrupt input; repeated `Execute` calls
      stay correct
- [ ] §18 — Input read only in `LoadData`; `Execute`/`ResetState` do not
      re-read input
- [ ] §18 — No extraneous code (unused/rarely-used code, leftover
      benchmarking/performance-measurement code removed)
- [ ] §18 — Every claim verified by actually running the checks in
      `llm/DEVELOPMENT.md`; CI-only checks explicitly reported as not run
      locally
- [ ] §18 — Clarifying questions asked before implementation when the
      task is ambiguous or touches code outside the request's scope (see
      `llm/CLAUDE.md`)
- [ ] §18 — All agent-created files live in `bin/` —
      todo/session/measurement logs, review reports, and every
      temp/scratch file (profiling data, dumps, one-off scripts,
      artifacts); never the opencode dir, repo root, `llm/`, or
      `graphify-out/` (see `llm/CLAUDE.md` §0)
- [ ] §18 — Todo/session discipline per `llm/CLAUDE.md` §0/§10 (S-rules):
      per-phase `bin/todo_<num>.md` files created first, real-time
      statuses, deleted when fully done — no exceptions for task size
