# todo_rules_15.md — Design review checklist: RULES.md §15 Logging

> **Template, not a live todo.** At the start of a **Design-mode** review,
> copy this file into `bin/`:
>
>     cp llm/todo_rules_15.md bin/todo_rules_15.md
>
> then walk it top to bottom as your own todo file for the §15 phase
> (`llm/AGENT.md` "Review workflow"; `llm/CLAUDE.md` §10 S2; `llm/PLAN.md`
> §2).
>
> - Mark a checkbox `in_progress` before reviewing it, `completed`
>   immediately after (S3); keep the `bin/todo_rules_15.md` file and the
>   in-session todo display in sync (same message).
> - A box is `completed` only after its verification actually ran —
>   command executed / log inspected / result recorded in the session log
>   (S5).
> - Record each box's verdict (**Pass / Fail / N/A / Needs-info**) with
>   `file:line` and *how verified* (build / profiling / inspection only)
>   in the review report (path per `llm/AGENT.md` "Review defaults"), not here.
> - Walk top to bottom, in order; every box gets a verdict (S4).
> - **Delete `bin/todo_rules_15.md` once every checkbox below is
>   `completed`** — a fully checked file is removed after the phase's
>   verification pass (S1; `llm/CLAUDE.md` §0 item 1; `llm/PLAN.md` §2). A
>   leftover completed file is a tracking failure.

Source: `llm/RULES.md` §15 "Logging".
Acceptance criterion: every §15 box has a verdict in the report, this
file is fully checked, and it is deleted.

## §15 — Logging

- [ ] §15 — Logs for debugging only, never end-user info; `{fmt}` syntax,
      never string concatenation
- [ ] §15 — Levels: TRACE (per-iteration), DEBUG (steps/key vars), INFO
      (progress), WARN (non-critical), ERROR (critical, often before
      `throw`), CRITICAL (crash/data-corruption)
- [ ] §15 — Custom types via templated `operator<<` (preferred) or
      `fmt::formatter`
- [ ] §15 — Expensive log preparation in hot loops guarded by
      `#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG`, kept at DEBUG+
