# Implement: `<algorithm>` (`<pattern>`)

> Fill-in template for implementing a new algorithm / primitive. Copy this
> file, replace every `<placeholder>`, delete guidance lines as you go.
> Workflow (todo files, phases): `llm/PLAN.md`. Requirements:
> `llm/RULES.md`. Commands (build, test, snapshot, format):
> `llm/DEVELOPMENT.md`. Performance targets: `llm/PERFORMANCE.md` +
> measurement protocol (`llm/PLAN.md` §5).

## 1. Spec (fill before writing any code)

- Name: `<algo>` (snake_case); class: `<AlgoClass>`; pattern:
  `<fd|ucc|dc|ar|md|nd|rfd|od|ind|…>`
- Paper: `<docs/papers/... or external reference>`
- Primitive, one sentence: `<what it discovers>`
- Options: `<name — type — default — meaning>`
- Output pattern object: `<RULES.md §17 — what the user must be able to do>`
- Format to match (past example): `<an existing algorithm/example to
  mirror, e.g. examples/basic/mining_<existing>.py and its snapshot layout>`
- Acceptance criteria (precise, checkable — these define "done"):
  - [ ] `<e.g. ctest -R "<algo-regex>" passes>`
  - [ ] `<e.g. example output matches the format of the named past example>`
  - [ ] `<e.g. bindings expose <operations> per RULES.md §17>`

> If anything above is unclear, ask the user **before** starting
> (`llm/CLAUDE.md` §1) — do not guess the spec.

## 2. Implementation steps (check off in order)

- [ ] Core implementation in `src/core/algorithms/<pattern>/<algo>/`
      (style: RULES.md §14; logging: §15; portability: §2–§5)
- [ ] CMake target via `desbordante_add_lib` + registration in
      `create_algo` (RULES.md §8)
- [ ] Exceptions / explanations (§11); option & exception types (§16)
- [ ] Standalone data structures, if any (§13)
- [ ] Python bindings — mechanism (§9) and required functionality (§10)
- [ ] Pattern object operations (§17)
- [ ] Unit tests `src/tests/unit/test_<algo>.cpp` via
      `desbordante_add_test`
- [ ] Usage example + snapshot (§12)
- [ ] Compilers & sanitizer matrix (§1, §7); cross-config behavior (§6)
- [ ] CI awareness — Python wheels (§19)

## 3. Verification chain (DEVELOPMENT.md §6, in order)

Run each check, record command + result in `bin/session_<YYYY-MM-DD>.md`.
**Conditional loop:** on failure → fix → re-run from the first failed
check; never skip a check or report it passed without running it.

- [ ] Build: `<command>` → `<result>`
- [ ] Targeted tests: `ctest --test-dir build -R "<algo-regex>"` →
      `<result>`
- [ ] Examples + snapshots: `pytest … -k <algo>` → `<result>`
- [ ] clang-format v22 on changed C++ files → `<result>`
- [ ] cmake-format on changed CMake files → `<result>`
- [ ] Graph refresh: `llm/graphify-update` → `<result>`
- [ ] Second-pass review: a fresh reviewer pass (or second model) checks
      the final output against the §1 acceptance criteria → `<findings
      fixed / explicitly waived>`

## 4. Definition of done

- [ ] All `llm/CLAUDE.md` §6 checkboxes ticked
- [ ] `bin/todo_<num>.md` for this task fully checked and deleted
- [ ] Session log up to date (`bin/session_<YYYY-MM-DD>.md`)
- [ ] Nothing committed or pushed unless explicitly asked
