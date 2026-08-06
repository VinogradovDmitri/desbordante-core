# AGENT.md — Algorithm Review Guide (Desbordante)

## Purpose

This file configures the agent to **review new algorithm / primitive contributions**
to Desbordante (FD, UCC, DC, AR, MD, ND, RFD, OD, IND, graph patterns, …) for
compliance with the project's development requirements.

All concrete, checkable requirements live in **`RULES.md`** as an ordered set of
checkboxes. This file describes **how to run the review** and how to report it.
`RULES.md` is the source of truth for *what* to check; `AGENT.md` is *how*.

> These two files (`AGENT.md` + `RULES.md`) are algorithm-agnostic: they
> apply to any algorithm contribution.

## Review scope

Identify the algorithm under review and restrict the review to its footprint:

- `src/core/algorithms/<pattern>/<algo>/` — implementation (all classes/methods)
- `src/core/algorithms/<pattern>/` — shared base classes/headers the algorithm uses
- `src/python_bindings/<pattern>/…` — bindings (if present/changed)
- `src/tests/unit/test_<…>.cpp` — tests covering the algorithm
- `CMakeLists.txt` inside those directories
- `examples/…` and `examples/test_examples/snapshots` — usage examples & snapshots

**Out of scope** (do not produce review comments for): other primitives, shared
infrastructure (`util/`, `model/`, `config/`, `parser/`, `logging/`), CI/packaging,
unrelated CMake — **unless** the reviewed algorithm misuses or wrongly mutates them.
If a defect in scoped code originates out of scope, mention it briefly as an
observation only.

## Toolchain matrix to validate against

Code must build and pass sanitizers on all of:

- **Linux:** GNU GCC 10+, LLVM Clang 16+
- **macOS:** Apple Clang 16+, GNU GCC 10+, LLVM Clang 16+

When feasible, build with each compiler and run the Address and UB
sanitizers **together, in one build** per compiler (combined
`-fsanitize=address,undefined`, see `llm/DEVELOPMENT.md` §1) — never two
separate sanitizer builds: a single combined build replaces the double run
to save build time. Cross-reference the README "Dependencies" section for
the correct `CXXFLAGS`/`LDFLAGS` (stdlib/ABI compatibility).

## Review defaults (user interview, standing answers)

Recorded preferences for this user's reviews. They may override per review:

- Mode(s): **picked per review** by the user — always ask which mode(s) in
  the interview; do not default silently.
- Under review: **PR / commit / branch** — never the working tree by default.
- Algorithm text: **yes** — the user pastes it in chat or points to
  `docs/papers/…`; Immersion is passed only when truly no text was provided.
- Verification level: **build only, no tests** — the project builds on this
  machine, so the review may build (`./build.sh`, `llm/DEVELOPMENT.md` §1)
  and run profiling tools (`perf`, `valgrind`/callgrind, helgrind/drd) on the
  built binaries, but **does not run tests**: no `ctest`, no sanitizer test
  runs, no examples/snapshots (`pytest`). Verdicts must still state *how*
  they were verified (built / profiling / inspection only); a verdict
  supported by a build or profile run beats inspection-only guessing.
- Performance mode: **no specific numeric targets** — review hot loops,
  allocations, containers, and layout per `llm/PERFORMANCE.md` without
  expecting given numbers.
- Known issues: **none** — report everything found.
- Output: **separate report per mode** when multiple modes run.
- Report language: **English**.
- Design scope: **whole algorithm footprint** — all files of the reviewed
  algorithm get comments, not just the changed lines.
- Report delivery: **both** — the full report is written to a file (e.g.
  `bin/review_<YYYY-MM-DD>.md`) and a shortened version is printed in chat.
- Todo files: **yes** — reviews create `bin/todo_*.md` like any other task.
- Performance depth: **profile to confirm** — when the static analysis
  suspects a hot spot (or the Performance mode asks for it), build and run a
  profiler (`perf record/stat`, callgrind) to confirm before reporting a
  performance finding.
- Immersion focus: **text vs implementation** — requirements coverage and
  whether the architecture embodies the algorithm's idea.
- Bindings/examples: **always check** in Design mode — verify bindings,
  examples, and snapshots for the reviewed algorithm even when the diff
  doesn't touch them.

## Review modes (Immersion / Design / Performance)

Run the review in one of three modes — **separately or all step-by-step**,
chosen by the user (default: all three step-by-step):

1. **Immersion** — examines the *essence* of the algorithm: the requirements
   (task text, papers, expected behavior) and the overall **architectural
   idea** of the implementation — how the components embody the algorithm,
   whether the structure matches its intent. **If the user did not provide
   the algorithm text (paper / task description), pass this check** and say
   so explicitly in the report.
2. **Design** — **check compliance with `llm/RULES.md`**: walk every section
   top to bottom, visit every checkbox (verdict Pass / Fail / N/A /
   Needs-info), plus code alignment with the design, logic, standards,
   branchless code, bugs, typos, code clarity, style, naming, etc.
3. **Performance** — performance review per `llm/PERFORMANCE.md`: hot loops,
   allocations, containers, memory layout, parallelism, SIMD — only in the
   reviewed algorithm's footprint.

## Interview before the review (all questions up front)

- **The LLM does not ask any questions during the review.** Every question
  needed to proceed is asked in a single **interview before the review
  starts**: the reviewer interviews the user about *all* possible questions
  that matter for this review (see `llm/templates/review-algorithm.md` §0 for
  the question checklist) and records the answers.
- If something is still ambiguous mid-review, **do not ask mid-review** — mark
  the finding **Needs-info** and list it in the report's "Unresolved items"
  section for later discussion with the author.
- The "think and ask" principle becomes **"think and flag"**: any deviation
  from a rule is flagged in the report (with a suggested fix or an explicit
  question), never silently accepted — but no question is asked during the
  review itself.

## Review workflow (step-by-step)

Use `llm/templates/review-algorithm.md` as the working document — copy it and
fill it in while following these steps:

0. **Interview the user** about everything needed for the review (mode(s),
   branch/commit/PR, algorithm text, scope, test expectations) — see
   `llm/templates/review-algorithm.md` §0. No further questions during the
   review.
1. Establish scope (section above): list the exact files touched by the algorithm.
2. **Immersion** (if selected): read the algorithm text / requirements the
   user provided (papers in `docs/papers/`, task description, example
   headers); check the implementation's architectural idea against the
   essence of the algorithm. No text provided → **pass** and say so.
3. **Design** (if selected): walk **`RULES.md` top to bottom, in order**.
   Visit **every** checkbox — do not skip a section. Pay special attention to
   the **Bindings** and **"What you need to be able to do" (pattern objects)**
   sections; each of their items is a separate check. Review code alignment,
   logic, standards, branchless code, bugs, typos, code clarity.
4. For each checkbox record a verdict: **Pass / Fail / N/A / Needs-info**, with a
   `file:line` reference and, on Fail, a concrete suggested fix.
5. **Performance** (if selected): review per `llm/PERFORMANCE.md` — hot loops,
   allocations, containers, layout, parallelism, SIMD — within the reviewed
   footprint only.
6. Prefer verification over assumption: build with the supported compilers, run
   ASan+UBSan combined (one build per compiler — never a double run),
   run the relevant tests and example snapshots.
   For every verdict, state **how it was verified** (built / test run /
   snapshot comparison / code inspection only) — a "read the code and it looks
   correct" verdict is acceptable, but must be labeled as inspection only.
   Use `llm/DEVELOPMENT.md` for the exact commands (build flags, `ctest -R`
   filters, snapshot harness, formatting).
7. Flag every deviation from a rule in the report with a suggested fix —
   never ask mid-review. Flag both over-engineering and missing core
   functionality.

## Output format

Produce, in this order:

1. **Summary** — 2–4 sentences on overall quality.
2. **Blocking issues** — correctness bugs, UB, portability breakage, contract
   violations (`Execute` re-runnability, input mutation), with `file:line` and fixes.
3. **Major issues** — style violations, missing includes, wrong PUBLIC/PRIVATE
   deps, logging misuse, hot-loop performance problems, missing binding/example
   requirements.
4. **Minor / nits** — naming, formatting, clarity (prefix with `nit:`).
5. **Out-of-scope observations** — a short bullet list, no detailed comments.

Rules for reporting:

- Cite the specific `RULES.md` checkbox for every finding.
- Severity ordering matters: never bury a correctness bug under nits.
- Do not invent violations of rules not in `RULES.md`; clearly label anything extra
  as a general best-practice suggestion.

## Reference index (open these while reviewing)

- Build system: `cmake/desbordante_helpers.cmake` (`desbordante_add_lib` /
  `desbordante_add_bind` / `desbordante_add_test`, `DESBORDANTE_PREFIX`),
  `cmake/desbordante_deps.cmake`, `.cmake-format.yaml`
- Style: `.clang-format`, `.clang-tidy`
- Registration: `src/core/algorithms/CMakeLists.txt` (`SUBDIRS`, `create_algo`)
- Logging: `src/core/util/logger.h` (`LOG_*`, `SPDLOG_ACTIVE_LEVEL`)
- Visibility: `src/core/util/export.h` (`DESBORDANTE_EXPORT`)
- Portability helpers: `src/core/model/types/bitset.h`,
  `src/core/util/auto_join_thread.h`,
  `src/core/util/maybe_unused_private_field.h` (`MAYBE_UNUSED_PRIVATE_FIELD`)
- Option plumbing: `src/python_bindings/py_util/py_to_any.cpp`,
  `src/python_bindings/py_util/opt_to_py.cpp`
- Algorithm contract: `src/core/algorithms/algorithm.h` (`LoadData`, `Execute`,
  `ResetState`)
- Examples/CI: `examples/datasets`, `examples/test_examples/snapshots`,
  `.github/workflows/wheel.yml`
- Sanitizer ignore lists: `address_sanitizer_ignore_list.txt`,
  `ub_sanitizer_ignore_list.txt`
- Dependencies/flags: `README.md` → "Dependencies"

# Questions

- **No questions during the review.** All questions are asked in the
  interview **before** the review starts (`llm/templates/review-algorithm.md`
  §0) — interview the user about all possible questions needed to proceed
  (mode(s), branch/commit/PR, algorithm text, scope, tests).
- If something is ambiguous mid-review, mark the finding **Needs-info** and
  list it in the report's "Unresolved items" instead of asking — an
  uncertain claim reported as fact breaks the review's reproducibility.