# todo_rules_9.md — Design review checklist: RULES.md §9 Bindings — mechanism

> **Template, not a live todo.** At the start of a **Design-mode** review,
> copy this file into `bin/`:
>
>     cp llm/todo_rules_9.md bin/todo_rules_9.md
>
> then walk it top to bottom as your own todo file for the §9 phase
> (`llm/AGENT.md` "Review workflow"; `llm/CLAUDE.md` §10 S2; `llm/PLAN.md`
> §2).
>
> - Mark a checkbox `in_progress` before reviewing it, `completed`
>   immediately after (S3); keep the `bin/todo_rules_9.md` file and the
>   in-session todo display in sync (same message).
> - A box is `completed` only after its verification actually ran —
>   command executed / log inspected / result recorded in the session log
>   (S5).
> - Record each box's verdict (**Pass / Fail / N/A / Needs-info**) with
>   `file:line` and *how verified* (build / profiling / inspection only)
>   in the review report (path per `llm/AGENT.md` "Review defaults"), not here.
> - Walk top to bottom, in order; every box gets a verdict (S4).
> - **Delete `bin/todo_rules_9.md` once every checkbox below is
>   `completed`** — a fully checked file is removed after the phase's
>   verification pass (S1; `llm/CLAUDE.md` §0 item 1; `llm/PLAN.md` §2). A
>   leftover completed file is a tracking failure.

Source: `llm/RULES.md` §9 "Bindings — mechanism".
Acceptance criterion: every §9 box has a verdict in the report, this
file is fully checked, and it is deleted.

## §9 — Bindings — mechanism

- [ ] §9 — Added via `desbordante_add_bind(<name> SRCS … LIBS …)` in
      `src/python_bindings/CMakeLists.txt`
- [ ] §9 — `LIBS` includes every library whose types are bound (plus
      `Boost::headers` if needed)
