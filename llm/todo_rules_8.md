# todo_rules_8.md — Design review checklist: RULES.md §8 CMake (target-based build)

> **Template, not a live todo.** At the start of a **Design-mode** review,
> copy this file into `bin/`:
>
>     cp llm/todo_rules_8.md bin/todo_rules_8.md
>
> then walk it top to bottom as your own todo file for the §8 phase
> (`llm/AGENT.md` "Review workflow"; `llm/CLAUDE.md` §10 S2; `llm/PLAN.md`
> §2).
>
> - Mark a checkbox `in_progress` before reviewing it, `completed`
>   immediately after (S3); keep the `bin/todo_rules_8.md` file and the
>   in-session todo display in sync (same message).
> - A box is `completed` only after its verification actually ran —
>   command executed / log inspected / result recorded in the session log
>   (S5).
> - Record each box's verdict (**Pass / Fail / N/A / Needs-info**) with
>   `file:line` and *how verified* (build / profiling / inspection only)
>   in the review report (path per `llm/AGENT.md` "Review defaults"), not here.
> - Walk top to bottom, in order; every box gets a verdict (S4).
> - **Delete `bin/todo_rules_8.md` once every checkbox below is
>   `completed`** — a fully checked file is removed after the phase's
>   verification pass (S1; `llm/CLAUDE.md` §0 item 1; `llm/PLAN.md` §2). A
>   leftover completed file is a tracking failure.

Source: `llm/RULES.md` §8 "CMake (target-based build)".
Acceptance criterion: every §8 box has a verdict in the report, this
file is fully checked, and it is deleted.

## §8 — CMake (target-based build)

- [ ] §8 — Correct target types: internal `OBJECT`, header-only umbrella
      `INTERFACE` (`FILE_SET HEADERS`), user-facing `LIBRARY`; no manual
      `add_executable`
- [ ] §8 — One `CMakeLists.txt` per directory with sources (or one target
      for the whole algorithm); each pulled in via `add_subdirectory` from
      its parent only
- [ ] §8 — Target via `desbordante_add_lib(NAME <TYPE>)`; `${NAME}` used
      afterward, never the literal name; main target named
      `<pattern>.<algo>`
- [ ] §8 — `target_sources` lists only `.cpp` files (except header-only
      targets)
- [ ] §8 — `PUBLIC` = deps used in headers; `PRIVATE` = deps used only in
      `.cpp`
- [ ] §8 — Desbordante deps use `${DESBORDANTE_PREFIX}::…`; link
      `::algos`, not `::create_algo` (cyclic dependency)
- [ ] §8 — Directory added to `SUBDIRS`
      (`src/core/algorithms/CMakeLists.txt`); main target in the
      `create_algo` dependency list
- [ ] §8 — No missing/superfluous deps ("file not found" = missing or
      wrongly PRIVATE — fix the linkage)
- [ ] §8 — All CMake files pass `cmake-format` with `.cmake-format.yaml`
