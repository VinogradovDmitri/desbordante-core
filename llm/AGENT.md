# AGENT.md — Algorithm Review Guide (Desbordante)

How to run reviews of algorithm/primitive contributions (FD, UCC, DC, AR,
MD, ND, RFD, OD, IND, graph patterns, …). Checkable requirements live in
`llm/RULES.md` (*what*); this file is *how*. Algorithm-agnostic.

## Review scope

Footprint (only these get review comments):
- `src/core/algorithms/<pattern>/<algo>/` + shared bases in
  `src/core/algorithms/<pattern>/`
- `src/python_bindings/<pattern>/…` (if present/changed)
- `src/tests/unit/test_<…>.cpp`; `CMakeLists.txt` in those directories
- `examples/…` + `examples/test_examples/snapshots`

**Out of scope** (no comments): other primitives, shared infrastructure
(`util/`, `model/`, `config/`, `parser/`, `logging/`), CI/packaging,
unrelated CMake — unless the reviewed algorithm misuses them; mention such
defects briefly as observations only.

## Toolchain matrix

Must build and pass sanitizers on: Linux GCC 10+, Linux LLVM Clang 16+,
macOS Apple Clang 16+, macOS GCC 10+, macOS LLVM Clang 16+. ASan+UBSan run
**together, one build per compiler** (`-fsanitize=address,undefined`,
`llm/DEVELOPMENT.md` §1) — never two separate sanitizer builds. Cross-check
README "Dependencies" for `CXXFLAGS`/`LDFLAGS`.

## Review defaults (user interview — standing answers, overridable per review)

- Modes: **picked per review** — always ask in the interview
- Target: **PR / commit / branch** (never the working tree by default)
- Algorithm text: provided (chat or `docs/papers/…`); Immersion passes only
  when truly none is given
- Verification: **build only, no tests** — may build (`./build.sh`) and run
  profiling tools (`perf`, `valgrind`/callgrind, helgrind/drd) on the built
  binaries; **no** `ctest`, sanitizer runs, or examples/snapshots
- Verdicts state how verified (built / profiling / inspection only); a
  profiled or built verdict beats inspection-only guessing
- Performance: no specific numeric targets — review hot loops, allocations,
  containers, layout per `llm/PERFORMANCE.md`
- Known issues: none — report everything
- Output: **separate report per mode**; language **English**
- Design scope: **whole algorithm footprint**, not just the diff
- Delivery: full report → `bin/review_<YYYY-MM-DD>.md` + shortened version
  in chat; reviews get `bin/todo_*.md` like any other task
- Perf depth: **profile to confirm** suspected hot spots before reporting
- Immersion focus: **text vs implementation** (requirements coverage +
  architecture)
- Bindings/examples: **always check** in Design mode, even if the diff
  doesn't touch them

## Review modes (Immersion / Design / Performance)

Run separately or all step-by-step (user picks; default: all three):

1. **Immersion** — the *essence* of the algorithm: requirements (task text,
   papers, expected behavior) vs the overall **architectural idea** of the
   implementation. **No algorithm text provided → pass this check** and say
   so explicitly in the report.
2. **Design** — **compliance with `llm/RULES.md`**: every section top to
   bottom, every checkbox (verdict Pass / Fail / N/A / Needs-info), plus
   code alignment, logic, standards, branchless code, bugs, typos, code
   clarity, style, naming.
3. **Performance** — per `llm/PERFORMANCE.md`: hot loops, allocations,
   containers, memory layout, parallelism, SIMD — footprint only.

## Interview before the review (all questions up front)

- **No questions during the review.** Everything needed is asked in a
  single interview **before** the review starts (checklist:
  `llm/templates/review-algorithm.md` §0); answers recorded.
- Ambiguous mid-review → mark **Needs-info**, list in the report's
  "Unresolved items" for after the review.
- "Think and ask" becomes **"think and flag"**: deviations are flagged in
  the report with a suggested fix, never silently accepted — but never
  asked mid-review.

## Review workflow

0. **Interview** the user (mode(s), target, algorithm text, scope, tests) —
   template §0. No further questions during the review.
1. Establish scope: exact files of the footprint.
2. **Immersion** (if selected): algorithm text vs implementation; no text →
   pass and say so.
3. **Design** (if selected): walk `llm/RULES.md` top to bottom, every
   checkbox — special attention to Bindings (§9–10) and Pattern objects
   (§17); code alignment, logic, standards, branchless code, bugs, typos,
   clarity.
4. Verdict per checkbox: **Pass / Fail / N/A / Needs-info** with
   `file:line` and, on Fail, a concrete suggested fix.
5. **Performance** (if selected): per `llm/PERFORMANCE.md`, footprint only.
6. Verify, don't assume: build; ASan+UBSan combined (one build per
   compiler); state how each verdict was verified; commands from
   `llm/DEVELOPMENT.md`.
7. Flag every deviation with a suggested fix; flag both over-engineering
   and missing core functionality.

## Output format (per mode)

1. **Summary** — 2–4 sentences on overall quality.
2. **Blocking** — correctness bugs, UB, portability breakage, contract
   violations (`Execute` re-runnability, input mutation) — `file:line` +
   fixes.
3. **Major** — style violations, missing includes, wrong PUBLIC/PRIVATE
   deps, logging misuse, hot-loop performance, missing binding/example
   requirements.
4. **Minor / nits** — naming, formatting, clarity (prefix with `nit:`).
5. **Out-of-scope observations** — short bullet list only.

Reporting rules: cite the specific `RULES.md` checkbox per finding;
severity ordering (never bury a bug under nits); don't invent violations —
label extras as best-practice suggestions.

## Reference index

- Build: `cmake/desbordante_helpers.cmake` (`desbordante_add_lib` /
  `desbordante_add_bind` / `desbordante_add_test`, `DESBORDANTE_PREFIX`),
  `cmake/desbordante_deps.cmake`, `.cmake-format.yaml`
- Style: `.clang-format`, `.clang-tidy`
- Registration: `src/core/algorithms/CMakeLists.txt` (`SUBDIRS`,
  `create_algo`)
- Logging: `src/core/util/logger.h`; visibility: `src/core/util/export.h`
  (`DESBORDANTE_EXPORT`)
- Portability: `src/core/model/types/bitset.h`,
  `src/core/util/auto_join_thread.h`,
  `src/core/util/maybe_unused_private_field.h`
- Option plumbing: `src/python_bindings/py_util/py_to_any.cpp`,
  `src/python_bindings/py_util/opt_to_py.cpp`
- Algorithm contract: `src/core/algorithms/algorithm.h` (`LoadData`,
  `Execute`, `ResetState`)
- Examples/CI: `examples/datasets`, `examples/test_examples/snapshots`,
  `.github/workflows/wheel.yml`
- Sanitizer ignore lists: `address_sanitizer_ignore_list.txt`,
  `ub_sanitizer_ignore_list.txt`; deps: README "Dependencies"

# Questions

- **No questions during the review.** All questions go in the pre-review
  interview (template §0); mid-review ambiguities → **Needs-info** +
  "Unresolved items" — an uncertain claim reported as fact breaks the
  review's reproducibility.
