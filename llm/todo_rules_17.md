# todo_rules_17.md — Design review checklist: RULES.md §17 Pattern objects (what you need to be able to do)

> **Template, not a live todo.** At the start of a **Design-mode** review,
> copy this file into `bin/`:
>
>     cp llm/todo_rules_17.md bin/todo_rules_17.md
>
> then walk it top to bottom as your own todo file for the §17 phase
> (`llm/AGENT.md` "Review workflow"; `llm/CLAUDE.md` §10 S2; `llm/PLAN.md`
> §2).
>
> - Mark a checkbox `in_progress` before reviewing it, `completed`
>   immediately after (S3); keep the `bin/todo_rules_17.md` file and the
>   in-session todo display in sync (same message).
> - A box is `completed` only after its verification actually ran —
>   command executed / log inspected / result recorded in the session log
>   (S5).
> - Record each box's verdict (**Pass / Fail / N/A / Needs-info**) with
>   `file:line` and *how verified* (build / profiling / inspection only)
>   in the review report (path per `llm/AGENT.md` "Review defaults"), not here.
> - Walk top to bottom, in order; every box gets a verdict (S4).
> - **Delete `bin/todo_rules_17.md` once every checkbox below is
>   `completed`** — a fully checked file is removed after the phase's
>   verification pass (S1; `llm/CLAUDE.md` §0 item 1; `llm/PLAN.md` §2). A
>   leftover completed file is a tracking failure.

Source: `llm/RULES.md` §17 "Pattern objects (what you need to be able to
do)".
Acceptance criterion: every §17 box has a verdict in the report, this
file is fully checked, and it is deleted.

## §17 — Pattern objects (what you need to be able to do)

- [ ] §17 — `get_*()` objects have independent lifetime (ideally plain
      strings/numbers); new patterns do not use `RelationalSchema`,
      `Column`, `Vertical`
- [ ] §17 — Serialization simple (tuple of standard/previously-bound
      types)
- [ ] §17 — `__eq__`/`__hash__` implemented; `__eq__` via `py::self ==
      py::self` (`operator==`)
- [ ] §17 — `__str__`/`__repr__` provided; a constructor matching
      `__repr__` available
- [ ] §17 — Miner output convertible to validator input directly, or via
      a single method call (Fd ↔ FdInput)
- [ ] §17 — Modeled close to the paper's definition; access convenient
      (e.g. column names, not indices); Unicode preserved
- [ ] §17 — Class roles separated (representation / internal /
      result-storage / input): representation user-facing, convertible to
      input; validator accepts representation or an easy input class;
      storage compact, yields representation (`transform_view`,
      `py::make_iterator`, `reference_internal`); algorithm holds a
      `shared_ptr` to storage, overwritten on rerun
