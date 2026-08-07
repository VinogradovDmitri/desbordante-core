# Review: `<algorithm>` (`<pattern>`)

> Fill-in template for reviewing an algorithm contribution. Copy this file,
> replace every `<placeholder>`, delete guidance lines as you go. Modes and
> interview-first policy: `llm/AGENT.md`. Requirements: `llm/RULES.md`.
> Commands: `llm/DEVELOPMENT.md`. **No questions during the review —
> everything is resolved in the interview below, up front.**

## 0. Interview (before the review — ask the user *all* of this)

> Standing defaults are in `llm/AGENT.md` "Review defaults" — only ask what
> deviates. Record the answers here; delete questions that don't apply.

- [ ] Mode(s): **Immersion / Design / Performance** — one or all three
      step-by-step?
- [ ] Under review: `<branch / commit / PR>` — which exactly?
- [ ] Algorithm text: provided where (`docs/papers/…` / pasted in chat)?
      **If none is provided, the Immersion check is passed** (state it in
      the report)
- [ ] Scope: user-specified or inferred from the diff?
- [ ] Tests: default is **build only, no tests** — build and run profiling
      tools (perf, valgrind/callgrind, helgrind/drd) on the binaries; no
      `ctest`, sanitizer runs, or examples/snapshots — or something
      different for this review?
- [ ] Performance: any numeric targets / hot spots / datasets of interest?
- [ ] Known issues the user is already aware of (not to re-report)?
- [ ] Output: separate report per mode (default) or combined?
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

## 3. Toolchain matrix results

| Config | Build | ASan+UBSan (combined) | Targeted tests | Verified by |
|---|---|---|---|---|
| Linux GCC 10+ | `<pass/fail/not run>` | `<…>` | `<…>` | `<exact command>` |
| Linux Clang 16+ | `<…>` | `<…>` | `<…>` | `<…>` |
| macOS Apple Clang 16+ | `<…>` | `<…>` | `<…>` | `<…>` |
| macOS GCC 10+ | `<…>` | `<…>` | `<…>` | `<…>` |
| macOS LLVM Clang 16+ | `<…>` | `<…>` | `<…>` | `<…>` |

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

Produce in this order (severity rules: `llm/AGENT.md` "Output format"):

### Blocking
`<correctness bugs, UB, portability breakage, contract violations —
file:line + suggested fix, or "none">`

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

- [ ] Re-checked every finding against its cited `RULES.md` checkbox and
      the severity ordering (blocking → major → nits) — fresh read, or a
      second reviewer/model. Findings fixed or explicitly waived: `<notes>`
