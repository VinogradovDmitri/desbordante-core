# AGENT.md — Algorithm Review Guide (Desbordante)

## Purpose

This file configures the agent to **review new algorithm / primitive contributions**
to Desbordante (FD, UCC, DC, AR, MD, ND, RFD, OD, IND, graph patterns, …) for
compliance with the project's development requirements.

All concrete, checkable requirements live in **`RULES.md`** as an ordered set of
checkboxes. This file describes **how to run the review** and how to report it.
`RULES.md` is the source of truth for *what* to check; `AGENT.md` is *how*.

> These two files (`AGENT.md` + `RULES.md`) are the generic successors that 
> apply to any algorithm.

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

When feasible, build with each compiler and run the UB and Address sanitizers under
both GCC and Clang. Cross-reference the README "Dependencies" section for the
correct `CXXFLAGS`/`LDFLAGS` (stdlib/ABI compatibility).

## Review workflow (step-by-step)

1. Establish scope (section above): list the exact files touched by the algorithm.
2. Read the referenced paper(s) in `docs/papers/` (or the example header) so
   correctness can be judged against the primitive's definition.
3. Walk **`RULES.md` top to bottom, in order**. Visit **every** checkbox — do not
   skip a section. Pay special attention to the **Bindings** and **"What you need
   to be able to do" (pattern objects)** sections; each of their items is a
   separate check.
4. For each checkbox record a verdict: **Pass / Fail / N/A / Needs-info**, with a
   `file:line` reference and, on Fail, a concrete suggested fix.
5. Prefer verification over assumption: build with the supported compilers, run
   ASan/UBSan on GCC and Clang, run the relevant tests and example snapshots.
   For every verdict, state **how it was verified** (built / test run /
   snapshot comparison / code inspection only) — a "read the code and it looks
   correct" verdict is acceptable, but must be labeled as inspection only.
   Use `llm/DEVELOPMENT.md` for the exact commands (build flags, `ctest -R`
   filters, snapshot harness, formatting).
6. Apply the **"think and ask"** principle: any deviation from a rule is acceptable
   only as the result of discussion, never a unilateral decision. Flag both
   over-engineering and missing core functionality, and raise questions when unsure.

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

- Ask at least one question before concluding a review if anything is
  ambiguous — open questions are expected, not a failure.
- Ask more rather than less (more is recommended); see `llm/CLAUDE.md` for the
  general ask-first policy.