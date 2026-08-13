# AGENT.md — Algorithm Review Guide (Desbordante)

How to review algorithm/primitive contributions (FD, UCC, DC, AR, MD,
ND, RFD, OD, IND, graph patterns, …). *What* to check: `llm/RULES.md`;
this file is *how*. Algorithm-agnostic.

## Review scope

Footprint (only these get comments):
- `src/core/algorithms/<pattern>/<algo>/` + shared bases in
  `src/core/algorithms/<pattern>/`
- `src/python_bindings/<pattern>/…` (if present/changed)
- `src/tests/unit/test_<…>.cpp`; `CMakeLists.txt` in those dirs
- `examples/…` + `examples/test_examples/snapshots`

Out of scope (no comments): other primitives, shared infrastructure
(`util/`, `model/`, `config/`, `parser/`, `logging/`), CI/packaging,
unrelated CMake — unless the algorithm misuses them; mention briefly
as observations only.

## Toolchain matrix

Must build and pass tests on: Linux GCC 10+, Linux LLVM Clang 16+,
macOS Apple Clang 16+, macOS GNU GCC 10+, macOS LLVM Clang 16+.
**Sanitizers
(ASan/UBSan) are CI-only — no local sanitizer builds.** Cross-check
README "Dependencies" for `CXXFLAGS`/`LDFLAGS`.

## Review defaults (standing answers — overridable per review)

- Modes: **collected by `make review`** (it prompts when run interactively,
  accepts any comma-separated subset). Do not re-ask in the interview — read
  the `Review modes:` line of `bin/session_brief.md`.
- Target: **current branch's configured upstream versus `HEAD` by default**;
  explicit `BASE=`/`HEAD=` revisions are allowed (never the working tree).
  Collected by `make review`; do not re-ask — read `bin/session_brief.md`.
- Algorithm text: provided (chat or `docs/papers/…`); Immersion passes
  only when truly none is given. **This one is asked in the interview** (the
  LLM needs the source to read/verify it).
- Output contract: **collected by `make review`** (`commits` / `patches` /
  `report`). Do not re-ask — read `bin/session_brief.md`. `commits` creates
  one focused commit per accepted finding plus `report.md`; `patches` creates
  one stable numbered patch per accepted finding plus `report.md`; `report`
  makes no code changes and puts each suggested fix in a fenced code block in
  `report.md`.
- Verification: **collected by `make review`** as a multi-select
  (`build` / `tests` / `python` / `snapshots` / `determinism` / `profiling`,
  default `build`). Do not re-ask — read the `Verification:` line of
  `bin/session_brief.md`.
- Measurement dataset (Performance only): **picked per review** — propose
  several datasets (vary size/cardinality/columns), user multi-selects; a
  temporary `src/tests/unit/test_<algo>_perf_probe.cpp` is created to drive
  the measurement and **deleted after all todos are done**
- Worktree: **all code-touching work** happens in a git worktree under
  `bin/` (`git worktree add bin/<name> <branch>`); free it
  (`git worktree remove --force bin/<name>`) after the last commit so the
  branch is available to the user (`llm/DEVELOPMENT.md` §1)
- Verdicts state how verified (built / profiling / inspection only);
  a profiled/built verdict beats inspection-only guessing
- Performance: no specific numeric targets — review hot loops,
  allocations, containers, layout per `llm/PERFORMANCE.md`
- Known issues: none — report everything
- Output: **separate report per review mode** when requested; language
  **English**
- Design scope: **whole footprint**, not just the diff
- Report file: default `bin/report.md` — the deliverable for all three output
  contracts. `report` includes fenced suggested fixes; `commits` and `patches`
  map every accepted finding to its delivery artifact. Reviews get
  `bin/todo_*.md` like any task.
- Preparation: run `make review` before the review. It writes the compact
  `bin/session_brief.md` and creates one phase todo per 1–3 hour slice. Use
  `MODE=commits|patches|report`, `HOURS=<estimate>`, `PHASES=<override>`,
  `BASE=<revision>`, and `HEAD=<revision>` as needed.
- Perf depth: **profile to confirm** suspected hot spots before
  reporting
- Immersion focus: **text vs implementation** (requirements coverage +
  architecture)
- Bindings/examples: **always check** in Design mode, even if the diff
  doesn't touch them

## Review modes (Immersion / Design / Performance)

Run separately or step-by-step (user picks; default: all three):

1. **Immersion** — essence: requirements (task text, papers, expected
   behavior) vs the **architectural idea** of the implementation. No
   algorithm text → pass and say so.
2. **Design** — compliance with `llm/RULES.md`: every section top to
   bottom, every checkbox (verdict Pass / Fail / N/A / Needs-info) +
   code alignment, logic, standards, branchless code, bugs, typos,
   clarity, style, naming.
3. **Performance** — per `llm/PERFORMANCE.md`: hot loops, allocations,
   containers, memory layout, parallelism, SIMD — footprint only.

## Interview before the review (all questions up front — none mid-review)

All questions go in a single interview **before** the review
(checklist: `llm/templates/review-algorithm.md` §0); answers recorded.
**No questions during the review.** Mid-review ambiguity → mark
**Needs-info**, list in the report's "Unresolved items" for after.
"Think and ask" → **"think and flag"**: deviations flagged in the
report with a suggested fix, never silently accepted — never asked
mid-review. An uncertain claim reported as fact breaks the review's
reproducibility.

The interview has **six parts** (all up front). Parts **1, 2, 3, 4, and 6 are
collected by `make review`** — do **not** ask them in the interview; run
`make review` (which prompts when interactive) or read an existing
`bin/session_brief.md`, and use its recorded values. Ask only the parts below
that `make review` does not cover:

1. **Mode(s)** — collected by `make review` (comma-separated subset of
   Immersion / Design / Performance). **If Immersion is selected**, ask how
   the algorithm text will be provided: pasted in chat / `docs/papers/…`
   path / none. No text → Immersion passes (state it).
2. **Target** — collected by `make review` (upstream vs `HEAD`, or explicit
   `BASE=`/`HEAD=`).
3. **Output contract** — collected by `make review` (`commits` / `patches` /
   `report`).
4. **Verification** — collected by `make review` (comma-separated subset of
   build / tests / python / snapshots / determinism / profiling).
5. **Measurement dataset** (Performance mode only — multiple choice, asked
   here): propose several datasets (vary size/cardinality/columns), user
   multi-selects; create a temporary
   `src/tests/unit/test_<algo>_perf_probe.cpp` to drive the measurement,
   and **delete it after all todos are done**.
6. **Effort and phases** — collected by `make review` (`HOURS=`/`PHASES=`).
   Keep every phase within 1–3 hours.

## Review workflow

0. **Copy PREWORK template** — `mkdir -p bin && cp llm/todo_0.md
   bin/todo_0.md`. Check section A (always applies) now; sections B/C
   are checked after the interview (they depend on the selected modes).
 1. **Interview** the user — only the parts `make review` does not cover:
    algorithm-text source (if Immersion), measurement dataset (if
    Performance), scope, known issues, and other notes. **Do not re-ask
    mode(s), target, output contract, verification, or effort — they are
    collected by `make review`.** Run `make review` (it prompts for those
    when interactive and records them in `bin/session_brief.md`), or read an
    existing brief. Track the interview in `bin/todo_tmp_1.md` (a temporary
    todo); **delete `bin/todo_tmp_1.md` when the interview is done**. No
    further questions during the review itself — mid-review ambiguity →
    Needs-info.
2. **Check remaining PREWORK sections** based on interview answers:
   Performance mode → section B (profiling tools); benchmark → section
   C (measurement protocol). If any check **fails**, **stop** and ask
   the user to run the relevant `llm/PREWORK.sh` section. Then **delete
   `bin/todo_0.md`** (`llm/CLAUDE.md` §0 item 0, §10 S6).
3. **Create all todo files** from the interview answers, then run them
   in order. Establish scope: exact files of the footprint. **Read the
   full diff** of the target (branch/commit/PR) before any verdict —
   distinguish intent from accident. **Graphify required:** run
   `llm/graphify-explain "<algo>"` and `llm/graphify-path "<algo>"
   "bindings"` before verdicts — structural questions answered by the
   graph beat greps (`llm/CLAUDE.md` §7). **Todo files per mode (S1–S2,
   `llm/CLAUDE.md` §10):** `make review` creates one phase for each
   selected mode, plus scope, verification, delivery/report, and any
   additional 1–3 hour review slices. Design phases transcribe
   `llm/RULES.md` §1–§19; Performance phases transcribe
   `llm/PERFORMANCE.md` §1–§13. Never copy generated todo files from
   `llm/`. Delete each `bin/` phase file only after all its boxes are
   verified (S1). **Mid-work questions OK** if the answer affects remaining
   todos — ask, then update the todos.
 4. **Worktree** (`commits`, `patches`, or Performance mode):
   `git worktree add bin/<name> <branch>`; build there with the
    `datasets/` symlink recipe (`llm/DEVELOPMENT.md` §1). `report` reviews
    need no worktree.
5. **Performance probe** (Performance mode only): create the temporary
   `src/tests/unit/test_<algo>_perf_probe.cpp` for the measurement
   dataset(s) chosen in interview part 5; **delete it after all todos
   are done**.
6. **Immersion** (if selected): algorithm text vs implementation; no
   text → pass and say so.
7. **Design** (if selected): walk `llm/RULES.md` top to bottom, every
   checkbox — special attention to Bindings (§9–10) and Pattern
   objects (§17); code alignment, logic, standards, branchless code,
   bugs, typos, clarity. **Before verdicts:** read
   `src/tests/unit/test_<algo>.cpp` — tests reveal intended behavior
   and edge cases. **Test coverage check:** do the pattern (unit) tests
   cover edge cases (empty input, single row/column, all-null,
   all-duplicate, all-unique, max cardinality, long strings,
   malformed CSV, Unicode, integer limits)? Missing coverage = a
   Design finding. **Cross-algorithm precedent:** for each structural
   concern (partitioning, caching, binding pattern, option plumbing),
   check how 1–2 similar existing algorithms handle it via
   `llm/graphify-explain` / `llm/graphify-path`; unexplained deviation
   from precedent is a finding (deviation with reason is not).
8. Verdict per checkbox: **Pass / Fail / N/A / Needs-info** with
   `file:line` and, on Fail, a concrete suggested fix.
9. **Performance** (if selected): per `llm/PERFORMANCE.md`, footprint
   only. **Determinism probe required** (`llm/DEVELOPMENT.md` §2):
   run the 30-process cross-process probe — a data race is a
   **Blocking** finding even if the perf review is otherwise clean.
   **Valgrind/helgrind/drd required:** `valgrind --tool=memcheck`
   (memory errors), `helgrind` and `drd` (data races, deadlocks) —
   any error is Blocking. Profiling tools (`perf`, callgrind) confirm
   suspected hot spots before reporting.
 10. **Delivery**: `commits` → fix each issue straight in the worktree,
     **one commit per issue** (single-line subject); at the end propose
     squashing and free the worktree (`git worktree remove --force
     bin/<name>`). `patches` → produce one stable numbered patch per
     accepted issue and verify the series. `report` → zero code changes;
     write `report.md` with quoted evidence and each suggested fix in a
     fenced code block.
11. Verify, don't assume: run the verification selected in interview
    part 4 (no local sanitizer builds — CI covers them); state how each
    verdict was verified; commands from `llm/DEVELOPMENT.md` §1 — read
    its "Environmental pitfalls" (never build in `/tmp`; datasets
    symlink recipe) before a worktree build.
12. Flag every deviation with a suggested fix; flag both
    over-engineering and missing core functionality.

## Output format (per mode)

1. **Summary** — 2–4 sentences on overall quality.
2. **Blocking** — correctness bugs, UB, portability breakage, contract
   violations (`Execute` re-runnability, input mutation) —
   `file:line` + fixes.
3. **Major** — style violations, missing includes, wrong
   PUBLIC/PRIVATE deps, logging misuse, hot-loop performance, missing
   binding/example requirements.
4. **Minor / nits** — naming, formatting, clarity (prefix `nit:`).
5. **Out-of-scope observations** — short bullets only.

### Finding evidence (`report` mode)
Every Fail verdict must include: **(a)** the `RULES.md` checkbox
citation, **(b)** the quoted code at `file:line`, **(c)** one
sentence on *why* it violates the rule, **(d)** a concrete suggested
fix. No quote → **Needs-info**, not Fail. Pass/N/A verdicts cite
`file:line` and how verified (build / profiling / inspection only).

### Output-contract distinction
- **`report`**: the report is the deliverable — every finding has citation,
  quoted code, rationale, and a concrete suggested fix in a fenced code
  block; zero code changes.
- **`commits`**: fix each issue straight in the worktree, one focused commit
  per issue, and map every commit to a report entry. Propose squashing at the
  end and free the worktree.
- **`patches`**: fix each issue and emit one stable numbered patch per issue;
  map every patch to a report entry and verify the complete patch series.

### Uncertainty rule (bright line)
If you cannot point to the exact line that violates the rule, it is
**Needs-info** — never Fail. An uncertain claim reported as Fail
breaks the review's reproducibility.

### Severity calibration
| Situation | Severity |
|---|---|
| UB, contract violation (`Execute` re-runnability, input mutation), portability breakage | **Blocking** always |
| Missing `const&` / copy in a **hot loop** | **Major** |
| Missing `const&` / copy in a **cold path** | **Minor** |
| Missing include — compiles everywhere | **Minor** |
| Missing include — breaks a toolchain config | **Blocking** |
| `.clang-format` style deviation | **Minor** always |
| Wrong PUBLIC/PRIVATE deps, logging misuse, hot-loop perf | **Major** |
| Missing binding/example requirement (§9–10, §12, §17) | **Major** |
| Naming, formatting, clarity | **Minor** (`nit:` prefix) |

Reporting: cite the specific `RULES.md` checkbox per finding; severity
ordering (never bury a bug under nits); don't invent violations —
label extras as best-practice suggestions.

### `report.md` per-issue template

`report.md` is a plain list of issues — **no sorting required**: each
new issue is appended as soon as it is confirmed, and its delivery
artifact (`commits`/`patches` modes) is produced at the same time.
Every issue entry follows this template:

```markdown
### <NN>. <Issue name>

- **Type**: `Blocking` | `Major` | `Minor` (`nit: <…>`) — per the severity
  table above
- **Files & ranges**: `<path>:<start>-<end>` (repeat for every affected
  file/range)

**Why it is an issue** — what the code does, why it is wrong, and proof
the user can verify by reading this section alone:
- <one sentence per fact: what the code does → why it is wrong>
- Proof — <navigation to it>, one of:
  - violated rule: `RULES.md` §N "<checkbox text>" (see verdict row
    `<path>:<line>`)
  - tool log: `bin/<helgrind|drd|valgrind|perf|…>.log`, lines <x-y> —
    <what it shows>
  - measurement files: `bin/<probe/measurement log>:<lines>` — <result>
  - Immersion text mismatch: "<quoted sentence from the provided text>"
    vs implementation at `<path>:<line>` — <the difference>

**Suggested fix** (per output mode — exactly one of):
- `report`: <fenced code block with the replacement>
- `commits`: commit `<hash>` — delivered
- `patches`: patch file `bin/patches/<NN>-<name>.patch` — delivered

**Perf impact** (performance issues only): `bin/<measurement file>:<lines>`
— before: <baseline time/mem>, after: <fixed time/mem>, gain: <+X% / ×N>.
```

Rules:
- Never write "must"/"should" without saying *why* — every requirement
  carries its reason (the violated rule, the observed log line, the
  text mismatch).
- Proof must be navigable: exact file, line range, or quoted sentence.
  "See the helgrind log" alone is not proof.
- New issues are appended immediately when found, in discovery order;
  do not re-sort the report later.

## Second pass (before reporting done)

A fresh pass (or a second reviewer/model) checking three things:
1. **False positives** — re-verify every Fail against the actual code;
   if the code does not violate the cited rule, downgrade to Pass or
   Needs-info.
2. **Severity calibration** — is each finding correctly tiered per the
   table above? Promote/demote as needed.
3. **Coverage** — every footprint file has at least one verdict;
   every `RULES.md` section has a row in the verdict table; every
   verification step selected in the interview has a result. Gaps →
   go back and fill them.

## Reference index

- Build: `cmake/desbordante_helpers.cmake` (`desbordante_add_lib` /
  `desbordante_add_bind` / `desbordante_add_test`,
  `DESBORDANTE_PREFIX`), `cmake/desbordante_deps.cmake`,
  `.cmake-format.yaml`
- Style: `.clang-format`, `.clang-tidy`
- Registration: `src/core/algorithms/CMakeLists.txt` (`SUBDIRS`,
  `create_algo`)
- Logging: `src/core/util/logger.h`; visibility:
  `src/core/util/export.h` (`DESBORDANTE_EXPORT`)
- Portability: `src/core/model/types/bitset.h`,
  `src/core/util/auto_join_thread.h`,
  `src/core/util/maybe_unused_private_field.h`
- Option plumbing: `src/python_bindings/py_util/py_to_any.cpp`,
  `src/python_bindings/py_util/opt_to_py.cpp`
- Algorithm contract: `src/core/algorithms/algorithm.h` (`LoadData`,
  `Execute`, `ResetState`)
- Examples/CI: `examples/datasets`,
  `examples/test_examples/snapshots`, `.github/workflows/wheel.yml`
  (CI also runs ASan/UBSan)
- deps: README "Dependencies"
