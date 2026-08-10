# todo_rules_12.md — Design review checklist: RULES.md §12 Usage example

> **Template, not a live todo.** At the start of a **Design-mode** review,
> copy this file into `bin/`:
>
>     cp llm/todo_rules_12.md bin/todo_rules_12.md
>
> then walk it top to bottom as your own todo file for the §12 phase
> (`llm/AGENT.md` "Review workflow"; `llm/CLAUDE.md` §10 S2; `llm/PLAN.md`
> §2).
>
> - Mark a checkbox `in_progress` before reviewing it, `completed`
>   immediately after (S3); keep the `bin/todo_rules_12.md` file and the
>   in-session todo display in sync (same message).
> - A box is `completed` only after its verification actually ran —
>   command executed / log inspected / result recorded in the session log
>   (S5).
> - Record each box's verdict (**Pass / Fail / N/A / Needs-info**) with
>   `file:line` and *how verified* (build / profiling / inspection only)
>   in the review report (path per `llm/AGENT.md` "Review defaults"), not here.
> - Walk top to bottom, in order; every box gets a verdict (S4).
> - **Delete `bin/todo_rules_12.md` once every checkbox below is
>   `completed`** — a fully checked file is removed after the phase's
>   verification pass (S1; `llm/CLAUDE.md` §0 item 1; `llm/PLAN.md` §2). A
>   leftover completed file is a tracking failure.

Source: `llm/RULES.md` §12 "Usage example".
Acceptance criterion: every §12 box has a verdict in the report, this
file is fully checked, and it is deleted.

## §12 — Usage example

- [ ] §12 — States the primitive with its source paper (name, authors,
      year, venue)
- [ ] §12 — Mentions other examples (mining/validation counterpart;
      exact/approximate) — breadth
- [ ] §12 — Defines the primitive on a real example; describes parameters
      and allowed values; mentions multiple algorithms if applicable
- [ ] §12 — Dataset printed ≤ 15 rows / ≤ 6 columns (smaller is better;
      larger only if unavoidable); stored in `examples/datasets`, path
      from repo root
- [ ] §12 — Searches/verifies with working techniques (iterate instances,
      get right part, print)
- [ ] §12 — Shows behavior change when key parameters/data change (find
      error → fix → recheck → better result)
- [ ] §12 — References the next example for this primitive; added to CI,
      `snapshot-*` updated
- [ ] §12 — Snapshot regenerated via the harness (`--snapshot-update`)
      then a plain rerun passes
- [ ] §12 — All supported built-in metrics described; user-defined-metric
      example if supported
- [ ] §12 — If randomized: stated with reason, seed fixed, and seed
      verified to reproduce on another machine (e.g. Colab)
- [ ] §12 — Error-finding shown for left and right parts (two examples if
      applicable); mentions experimentation (tuning, typo hunting)
- [ ] §12 — Language polished; blank-line-separated paragraphs; key
      points optionally highlighted
