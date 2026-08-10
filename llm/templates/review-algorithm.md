# Review: `<algorithm>` (`<pattern>`)

> Fill-in template for reviewing an algorithm contribution. Copy this file,
> replace every `<placeholder>`, delete guidance lines as you go. Modes and
> interview-first policy: `llm/AGENT.md`. Requirements: `llm/RULES.md`.
> Commands: `llm/DEVELOPMENT.md`. **No questions during the review —
> everything is resolved in the interview below, up front.**

## 0. Interview (before the review — ask the user *all* of this)

> Five parts, all up front. Standing defaults are in `llm/AGENT.md`
> "Review defaults" — confirm what deviates. Record the answers below;
> delete parts that don't apply. **No questions during the review.**

- [ ] **1. Mode(s)** (multiple choice): Immersion / Design / Performance —
      one, several, or all three step-by-step? → `<answer>`
  - [ ] *(If Immersion)* algorithm text provided where: pasted in chat /
        `docs/papers/…` path / none? **None → Immersion passes** (state it
        in the report)
- [ ] **2. Target** (single choice): branch / commit / PR — which exactly?
      → `<answer>`
- [ ] **3. Delivery** (single choice): (a) fix-and-commit in a git worktree
      under `bin/` on a branch, or (b) report-only — no code changes, write
      `bin/report_<YYYY-MM-DD_HHMM>.txt`? → `<answer>`
- [ ] **4. Verification** (multiple choice): build / pattern (unit) tests
      (`ctest -R`) / Python tests / snapshots — which to run? (default:
      build only) → `<answer>`
- [ ] **5. Measurement dataset** (Performance only — multiple choice):
      proposed datasets `<list>`; user picks `<selection>`; a temporary
      `src/tests/unit/test_<algo>_perf_probe.cpp` is created and **deleted
      after all todos are done** → `<answer>`
- [ ] Scope: user-specified or inferred from the diff? → `<answer>`
- [ ] Known issues the user is already aware of (not to re-report)?
      → `<answer>`
- [ ] Anything else relevant? `<notes>`

Answers: `<record them here>`

## 1. Modes run

- [ ] **Immersion** — algorithm essence + architectural idea vs
      requirements (pass if no algorithm text provided) → `<verdict>`
- [ ] **Design** — compliance with `RULES.md` (every section, every
      checkbox), code alignment, logic, standards, branchless code, bugs,
      typos, clarity → `<verdict>`
- [ ] **Performance** — per `llm/PERFORMANCE.md` → `<verdict>`

## 2. Scope

- Under review: `<branch / commit / PR>`
- Paper(s): `<docs/papers/... or "none">`
- Footprint (exact files — nothing else gets comments):
  - `src/core/algorithms/<pattern>/<algo>/` — `<files>`
  - `src/python_bindings/<pattern>/…` — `<files or "none">`
  - `src/tests/unit/test_<…>.cpp` — `<files>`
  - `CMakeLists.txt` — `<which>`
  - `examples/…` + `examples/test_examples/snapshots` — `<files>`

### Coverage matrix (every footprint file × reviewed × mode)

| File | Immersion | Design | Performance | Verdict |
|---|---|---|---|---|
| `<file path>` | Y/N | Y/N | Y/N | `<Pass/Fail/N/A>` |
| `<…>` | | | | |

## 3. Toolchain matrix results

Sanitizers are covered by CI (GitHub Actions) — no local sanitizer builds.

| Config | Build | Targeted tests | Verified by |
|---|---|---|---|
| Linux GCC 10+ | `<pass/fail/not run>` | `<…>` | `<exact command>` |
| Linux Clang 16+ | `<…>` | `<…>` | `<…>` |
| macOS Apple Clang 16+ | `<…>` | `<…>` | `<…>` |
| macOS GCC 10+ | `<…>` | `<…>` | `<…>` |
| macOS LLVM Clang 16+ | `<…>` | `<…>` | `<…>` |

> If a config was not run, say so explicitly — do not claim it passed
> (`llm/CLAUDE.md` §5).

## 4. RULES.md checklist verdicts

Walk `RULES.md` top to bottom, every section. Verdicts: **Pass / Fail /
N/A / Needs-info** — each with `file:line` and *how verified* (build /
profiling / inspection only).

| RULES.md § | Verdict | `file:line` | Verified by |
|---|---|---|---|
| 1. Supported compilers & build | `<…>` | `<…>` | `<…>` |
| 2–5. Portability | `<…>` | `<…>` | `<…>` |
| 6. Cross-configuration behavior | `<…>` | `<…>` | `<…>` |
| 7. Sanitizers | `<…>` | `<…>` | `<…>` |
| 8. CMake | `<…>` | `<…>` | `<…>` |
| 9–10. Bindings | `<…>` | `<…>` | `<…>` |
| 11. Exceptions / explanations | `<…>` | `<…>` | `<…>` |
| 12. Usage example | `<…>` | `<…>` | `<…>` |
| 13. Standalone data structures | `<…>` | `<…>` | `<…>` |
| 14–15. C++ style, logging | `<…>` | `<…>` | `<…>` |
| 16. Option & exception types | `<…>` | `<…>` | `<…>` |
| 17. Pattern objects | `<…>` | `<…>` | `<…>` |
| 18. Development philosophy | `<…>` | `<…>` | `<…>` |
| 19. CI awareness — wheels | `<…>` | `<…>` | `<…>` |

## 5. Findings

Produce in this order (severity rules + evidence: `llm/AGENT.md`
"Output format"). **Report-only mode:** every Fail must include
citation + quoted code + why + suggested fix. **Fix-and-commit mode:**
fix each issue, one commit per issue, propose squash at end — no
quoted-code report.

### Blocking
`<correctness bugs, UB, portability breakage, contract violations,
data races — file:line + quoted code + suggested fix, or "none">`

### Major
`<style/CMake/logging/binding/example/performance issues, or "none">`

### Minor / nits
`<prefixed with "nit:", or "none">`

### Out-of-scope observations
`<short bullets only, or "none">`

## 6. Unresolved items (marked Needs-info mid-review)

- [ ] `<item>` — anything ambiguous during the review was marked
      **Needs-info** in §4/§5; no questions were asked mid-review. These
      are discussed with the user after the review.

## 7. Second pass (before submitting)

A fresh pass (or second reviewer/model) checking three things
(`llm/AGENT.md` "Second pass"):

- [ ] **False positives** — re-verified every Fail against the actual
      code; any that don't violate the cited rule downgraded to Pass or
      Needs-info → `<notes>`
- [ ] **Severity calibration** — each finding correctly tiered per the
      severity table; promoted/demoted as needed → `<notes>`
- [ ] **Coverage** — every footprint file has ≥1 verdict; every
      `RULES.md` section has a row in the §4 table; every verification
      step from the interview has a result; gaps filled → `<notes>`

## 8. Not checked

Explicit list of what was **not** run, with reason (per `llm/CLAUDE.md`
§5 — "say what you did not verify"):

- [ ] Footprint files not reviewed: `<files or "none">`
- [ ] `RULES.md` sections skipped: `<sections or "none">`
- [ ] Verification not run (from interview part 4): `<steps or "none">`
      — reason: `<why>`
- [ ] Toolchain configs not built: `<configs or "none">`
- [ ] Profiling tools not run: `<tools or "none">`
