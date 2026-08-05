# Review: `<algorithm>` (`<pattern>`)

> Fill-in template for reviewing an algorithm contribution. Copy this file,
> replace every `<placeholder>`, and delete guidance lines (like this one) as
> you go. Workflow and output rules: `llm/AGENT.md`. Requirements under check:
> `llm/RULES.md`. Exact commands: `llm/DEVELOPMENT.md`.

## 1. Scope

- Under review: `<branch / commit / PR>`
- Paper(s): `<docs/papers/... or "none — correctness judged from example header">`
- Footprint (exact files reviewed — nothing else gets review comments):
  - `src/core/algorithms/<pattern>/<algo>/` — `<files>`
  - `src/python_bindings/<pattern>/…` — `<files or "none">`
  - `src/tests/unit/test_<…>.cpp` — `<files>`
  - `CMakeLists.txt` — `<which ones>`
  - `examples/…` + `examples/test_examples/snapshots` — `<files>`

## 2. Toolchain matrix results

| Config | Build | ASan+UBSan | Targeted tests | Verified by (command) |
|---|---|---|---|---|
| Linux GCC 10+ | `<pass/fail/not run>` | `<…>` | `<…>` | `<exact command>` |
| Linux Clang 16+ | `<…>` | `<…>` | `<…>` | `<…>` |
| macOS Apple Clang 16+ | `<…>` | `<…>` | `<…>` | `<…>` |
| macOS GCC 10+ | `<…>` | `<…>` | `<…>` | `<…>` |
| macOS LLVM Clang 16+ | `<…>` | `<…>` | `<…>` | `<…>` |

> If a config was not run, say so explicitly — do not claim it passed
> (`llm/CLAUDE.md` §5).

## 3. RULES.md checklist verdicts

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

## 4. Findings

Produce in this order (severity rules: `llm/AGENT.md` "Output format"):

### Blocking
`<correctness bugs, UB, portability breakage, contract violations — file:line + suggested fix, or "none">`

### Major
`<style/CMake/logging/binding/example/performance issues, or "none">`

### Minor / nits
`<prefixed with "nit:", or "none">`

### Out-of-scope observations
`<short bullets only, or "none">`

## 5. Open questions for the author

- [ ] `<question>` — mandatory if anything was ambiguous; open questions are
      expected, not a failure. Mark any uncertain finding as **Needs-info**
      above instead of asserting it.

## 6. Second pass (before submitting)

- [ ] Re-checked every finding against its cited `RULES.md` checkbox and the
      severity ordering (blocking → major → nits) — fresh read, or a second
      reviewer/model. Findings fixed or explicitly waived: `<notes>`
