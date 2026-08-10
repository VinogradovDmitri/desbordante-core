# todo_rules_10.md — Design review checklist: RULES.md §10 Bindings — required functionality

> **Template, not a live todo.** At the start of a **Design-mode** review,
> copy this file into `bin/`:
>
>     cp llm/todo_rules_10.md bin/todo_rules_10.md
>
> then walk it top to bottom as your own todo file for the §10 phase
> (`llm/AGENT.md` "Review workflow"; `llm/CLAUDE.md` §10 S2; `llm/PLAN.md`
> §2).
>
> - Mark a checkbox `in_progress` before reviewing it, `completed`
>   immediately after (S3); keep the `bin/todo_rules_10.md` file and the
>   in-session todo display in sync (same message).
> - A box is `completed` only after its verification actually ran —
>   command executed / log inspected / result recorded in the session log
>   (S5).
> - Record each box's verdict (**Pass / Fail / N/A / Needs-info**) with
>   `file:line` and *how verified* (build / profiling / inspection only)
>   in the review report (path per `llm/AGENT.md` "Review defaults"), not here.
> - Walk top to bottom, in order; every box gets a verdict (S4).
> - **Delete `bin/todo_rules_10.md` once every checkbox below is
>   `completed`** — a fully checked file is removed after the phase's
>   verification pass (S1; `llm/CLAUDE.md` §0 item 1; `llm/PLAN.md` §2). A
>   leftover completed file is a tracking failure.

Source: `llm/RULES.md` §10 "Bindings — required functionality".
Acceptance criterion: every §10 box has a verdict in the report, this
file is fully checked, and it is deleted.

## §10 — Bindings — required functionality

- [ ] §10 — Instances obtainable in Python; iterable one-by-one with
      total count
- [ ] §10 — Each instance printable directly (`to_string`)
- [ ] §10 — Processable by parts (left part, right part, enumerate right
      attributes, thresholds X/Y, …)
- [ ] §10 — Comparable and hashable / placeable in a set (set
      intersect/subtract)
- [ ] §10 — Constructible; miner and validator objects coincide (mined
      object inserted straight into the validator)
- [ ] §10 — Serializable/deserializable preserving maximum information
      (e.g. schema)
- [ ] §10 — Stubs updated (Desbordante/desbordante-stubs); console/CLI
      output considered (Desbordante/desbordante-cli)
- [ ] §10 — User-defined metric (Python) passable into the C++ core where
      applicable
- [ ] §10 — Conversion strategy considered for objects with vs. without
      extra metrics (e.g. CFD confidence/support)
