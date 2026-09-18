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
- Options: `<name — type — default — meaning; every C++ param must be
  reachable from Python as a load_data/execute option (RULES.md §10);
  names from config::names, descriptions from config::descriptions;
  integer defaults with the exact suffix, e.g. 0U (RULES.md §16)>`
- Output pattern object: `<RULES.md §17 — what the user must be able to do>`
- Representation split: `<dependency representation in its own file(s),
  algorithm class separate — e.g. fd.h vs fd_algorithm.h (RULES.md §17)>`
- Stages: `<LoadData params vs Execute params; setting one may reveal more;
  stage done when GetNeededOptions() is empty (RULES.md §18)>`
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
      (style: RULES.md §14 — EOF newline, full variable names, minimal
      comments, no LLM dumps, `[[nodiscard]]` on guard/future returns, no
      `catch (...)` / `<bits/stdc++.h>` / `int`-for-bool; logging: §15;
      portability: §2–§5)
- [ ] Stage configuration: `RegisterOption` + `MakeOptionsAvailable` /
      `MakeExecuteOptsAvailable`, one-by-one `SetOption`,
      `GetNeededOptions()`-empty = done (§18); NO pipelines — never
      `ExcludeOptions` / `SetExternalOption` / `GetExternalTypeIndex` /
      `AddSpecificNeededOptions` / `ExecutePrepare`; chain via
      output-as-option
- [ ] Input via `TypedColumnData` / `GetColumnData`, never manual table
      reads; keep `IDatasetStream` out of algorithm logic; reuse
      `util/` / `model/` / `parser/` helpers first (§18); option errors as
      `config::ConfigurationError` (§16)
- [ ] CMake target via `desbordante_add_lib` + registration in
      `create_algo` (RULES.md §8 — new entries alphabetically sorted)
- [ ] Exceptions / explanations (§11); option & exception types (§16)
- [ ] Standalone data structures, if any (§13 — extractable independently
      of the algorithm, self-contained, no performance harm)
- [ ] Python bindings — mechanism (§9: `pybind11::pickle`, no
      `gil_scoped_acquire` — module is `mod_gil_not_used()`) and required
      functionality (§10: every param exposed as an option)
- [ ] Pattern object operations (§17)
- [ ] Unit tests `src/tests/unit/test_<algo>.cpp` via
      `desbordante_add_test`
- [ ] Usage example + snapshot (§12)
- [ ] Compilers matrix (§1); cross-config behavior (§6)
- [ ] CI awareness — Python wheels (§19)

> Configuration reference (RULES.md §16/§18 are normative; precedent:
> `src/core/algorithms/md/md_verifier/md_verifier.cpp`):
> - Python: `Algo()` → `load_data(**opts)` → `execute(**opts)` →
>   `get_*()`; extra kwargs for unneeded options are ignored.
>   `_set_option` / `_get_needed_options` are CLI-only internals.
> - C++ tests: `ConfigureFromMap` / `CreateAndLoadAlgorithm`
>   (`src/core/algorithms/algo_factory.h`) with exact-typed
>   `StdParamsMap` values — never hand-rolled configuration loops.
> - Option chaining:
>   `RegisterOption(Option{&field_, kName, kDName, <default>}
>       .SetNormalizeFunc(normalize).SetValueCheck(validate)
>       .SetConditionalOpts({{cond1, {kOpt2}}, {{}, {kOpt3}}}));`
> - Common scenarios: compare several params → order via
>   `SetConditionalOpts`, check in the last option of the chain; one-of →
>   sentinel default + condition exposing the alternative, or a selector
>   option.

## 3. Verification chain (DEVELOPMENT.md §6, in order)

Run each check, record command + result in `bin/session_<YYYY-MM-DD>.md`
(per `llm/DEVELOPMENT.md` §6 — conditional loop on failure).

- [ ] Build: `<command>` → `<result>`
- [ ] Targeted tests: `ctest --test-dir build -R "<algo-regex>"` →
      `<result>`
- [ ] Examples + snapshots: `pytest … -k <algo>` → `<result>`
- [ ] clang-format v22 on changed C++ files → `<result>`
- [ ] cmake-format on changed CMake files → `<result>`
- [ ] Graph staleness check (`find src docs llm -newer
      graphify-out/graph.json -print -quit`); if stale, warned the user
      (§7) → `<result>`
- [ ] Second-pass review: a fresh reviewer pass (or second model) checks
      the final output against the §1 acceptance criteria → `<findings
      fixed / explicitly waived>`

## 4. Definition of done

- [ ] All `llm/CLAUDE.md` §6 checkboxes ticked
- [ ] `bin/todo_<num>.md` for this task fully checked and deleted
- [ ] Session log up to date (`bin/session_<YYYY-MM-DD>.md`)
- [ ] Nothing committed or pushed unless explicitly asked; commit message
      = single-line subject, no description (repo convention)
