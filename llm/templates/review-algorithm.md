# Review: `<algorithm>` (`<pattern>`)

> Fill-in template for reviewing an algorithm contribution. Copy this file,
> replace every `<placeholder>`, and delete guidance lines (like this one) as
> you go. Modes and interview-first policy: `llm/AGENT.md`. Requirements
> under check: `llm/RULES.md`. Exact commands: `llm/DEVELOPMENT.md`.
> **No questions are asked during the review — everything is resolved in the
> interview below, up front.**

## 0. Interview (before the review — ask the user *all* of this)

> Ask everything needed to proceed in one go; record the answers here. Skip
> questions the user already answered; delete the ones that don't apply.
> **Standing defaults are recorded in `llm/AGENT.md` "Review defaults"** —
> only ask what deviates from them.

- [ ] Mode(s): **Immersion / Design / Performance** — one of them, or all
      three step-by-step?
- [ ] Under review: `<branch / commit / PR / working tree>` — which exactly?
- [ ] Algorithm text: will the user provide the task text / paper (where —
      `docs/papers/…`)? **If no text is provided, the Immersion check is
      passed** (state it in the report).
- [ ] Scope: exact files / algorithm footprint — user-specified or inferred
      from the diff?
- [ ] Tests: default is **build only, no tests** — build the project and run
      profiling tools (perf, valgrind/callgrind, helgrind/drd) on the
      binaries; no `ctest`, sanitizer runs, or examples/snapshots — or does
      the user want something different for this review?
- [ ] Performance expectations: any numeric targets / known hot spots /
      datasets the user cares about (for Performance mode)?
- [ ] Known issues: anything the user is already aware of / doesn't want
      re-reported?
- [ ] Output: one combined report or a separate report per mode?
- [ ] Anything else relevant to this specific review? `<notes>`

Answers: `<record them here>`

## 1. Modes run

- [ ] **Immersion** — algorithm essence + architectural idea vs requirements
      (pass if no algorithm text was provided) → `<verdict + notes>`
- [ ] **Design** — compliance with `RULES.md` (every section, every
      checkbox), code alignment, logic, standards, bugs,
      typos, clarity → `<verdict + notes>`
- [ ] **Performance** — per `llm/PERFORMANCE.md` → `<verdict + notes>`

## 2. Scope

- Under review: `<branch / commit / PR>`
- Paper(s): `<docs/papers/... or "none — correctness judged from example header">`
- Footprint (exact files reviewed — nothing else gets review comments):
  - `src/core/algorithms/<pattern>/<algo>/` — `<files>`
  - `src/python_bindings/<pattern>/…` — `<files or "none">`
  - `src/tests/unit/test_<…>.cpp` — `<files>`
  - `CMakeLists.txt` — `<which ones>`
  - `examples/…` + `examples/test_examples/snapshots` — `<files>`

## 3. Toolchain matrix results

| Config | Build | ASan+UBSan | Targeted tests | Verified by (command) |
|---|---|---|---|---|
| Linux GCC 10+ | `<pass/fail/not run>` | `<…>` | `<…>` | `<exact command>` |
| Linux Clang 16+ | `<…>` | `<…>` | `<…>` | `<…>` |
| macOS Apple Clang 16+ | `<…>` | `<…>` | `<…>` | `<…>` |
| macOS GCC 10+ | `<…>` | `<…>` | `<…>` | `<…>` |
| macOS LLVM Clang 16+ | `<…>` | `<…>` | `<…>` | `<…>` |

> If a config was not run, say so explicitly — do not claim it passed
> (`llm/CLAUDE.md` §5).

## 4. RULES.md checklist verdicts

Walk `RULES.md` top to bottom, every section. Verdicts:
**Pass / Fail / N/A / Needs-info** — each with `file:line` and *how verified*
(build / test run / snapshot comparison / inspection only).

| RULES.md § | Verdict | `file:line` | Verified by |
|---|---|---|---|
| 1. Supported compilers & build | `<…>` | `<…>` | `<…>` |
| 2–5. Portability (extensions, namespaces, template, ABI) | `<…>` | `<…>` | `<…>` |
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
| 18. Development philosophy (algorithm contract) | `<…>` | `<…>` | `<…>` |
| 19. CI awareness — wheels | `<…>` | `<…>` | `<…>` |

## 5. Findings

Produce in this order (severity rules: `llm/AGENT.md` "Output format"):

### Blocking
`<correctness bugs, UB, portability breakage, contract violations — file:line + suggested fix, or "none">`

### Major
`<style/CMake/logging/binding/example/performance issues, or "none">`

### Minor / nits
`<prefixed with "nit:", or "none">`

### Out-of-scope observations
`<short bullets only, or "none">`

## 6. Unresolved items (marked Needs-info mid-review)

- [ ] `<item>` — anything ambiguous during the review was marked **Needs-info**
      in §4/§5; no questions were asked mid-review. These items are
      discussed with the user after the review.

## 7. Second pass (before submitting)

- [ ] Re-checked every finding against its cited `RULES.md` checkbox and the
      severity ordering (blocking → major → nits) — fresh read, or a second
      reviewer/model. Findings fixed or explicitly waived: `<notes>`
