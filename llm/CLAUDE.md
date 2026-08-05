# CLAUDE.md

Behavioral guidelines to reduce common LLM coding mistakes. Merge with project-specific instructions as needed.

**Tradeoff:** These guidelines bias toward caution over speed. For trivial tasks, use judgment.

Performance tasks: read `llm/PERFORMANCE.md` (optimization checklist) and
`llm/PLAN.md` §5 (measurement protocol) before touching code.

## 0. Startup checklist — do these in order, on any machine

1. **Phase 0 — create all `bin/todo_<num>.md` files** (`llm/PLAN.md` §2): the
   first task is **always** to create all todo files up front — one per phase,
   for **every** task even small ones (a small task gets just `bin/todo_1.md`).
   Follow-up tasks go into them (e.g. the next tasks into `bin/todo_1.md`).
2. **Environment bootstrap** (`llm/DEVELOPMENT.md` §0, fresh machine only):
   create `.venv`, install all requirements, install `uv` and
   `cmake-format`/`clang-format` — then verify.
3. **Knowledge graph** (§7): ensure `graphify-out/graph.json` exists and is
   current — create it if missing, `bin/graphify-update` if stale — then
   consult it.
4. **Build parallelism** (§8): compute CNT_CPU_CORE on this machine and build
   with `-j<CNT_CPU_CORE / 2>` for tests, performance, and just-check builds
   alike.

## 1. Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
- For any task — **even a small one** — **ask at least one clarifying question
  first** — about intent, scope, or success criteria, so the job is done
  comprehensively. Never silently pick among interpretations.
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them - don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.
- Propose a brief plan for multi-step tasks and let the user confirm before
  editing multiple files.

## 2. Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask yourself: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

## 3. Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it - don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

The test: Every changed line should trace directly to the user's request.

## 4. Goal-Driven Execution

**Define success criteria. Loop until verified.**

Transform tasks into verifiable goals:
- "Add validation" → "Write tests for invalid inputs, then make them pass"
- "Fix the bug" → "Write a test that reproduces it, then make it pass"
- "Refactor X" → "Ensure tests pass before and after"

For multi-step tasks, state a brief plan:
```
1. [Step] → verify: [check]
2. [Step] → verify: [check]
3. [Step] → verify: [check]
```

Strong success criteria let you loop independently. Weak criteria ("make it work") require constant clarification.

## 5. Verify Before Done

**Never declare completion on assumption. Run the checks.**

- Before stopping, run every applicable check from `llm/DEVELOPMENT.md`
  (build, targeted `ctest`, example `pytest` + snapshots, clang-format,
  cmake-format).
- Report each check with the exact command and its result
  ("`ctest --test-dir build -R "<algo-regex>"` → all N tests passed").
- If a check cannot be run locally (e.g. clang-tidy, typos — CI-only), state
  that explicitly. Do not claim it passed.
- Explicitly say what you did **not** verify.
- Double-check your work before ending: re-read your diff and re-run the
  affected checks if anything changed in between.

## 6. Definition of Done

Before reporting a task as done, all of the following must hold:

- [ ] Change is surgical — every changed line traces to the request
- [ ] Code formatted (`clang-format` v22 on changed C++ files)
- [ ] CMake files formatted if touched (`cmake-format --check`)
- [ ] Build passes (same build flags as before the change)
- [ ] Targeted tests pass (`ctest --test-dir build -R "<algo>"`)
- [ ] Python examples and snapshots pass (`pytest ... -k <algo>`; snapshots
      regenerated via the harness and re-verified if output changed)
- [ ] Knowledge graph refreshed — `bin/graphify-update` run after code
      changes (see §7)
- [ ] Checks reported with commands + results; CI-only checks flagged
- [ ] Nothing committed — commits happen only when explicitly asked
- [ ] Nothing pushed — `git push` is **denied** in every session, no exceptions
      (no force-push either); all work stays local

## 7. Knowledge Graph (graphify)

The repo has a merged knowledge graph of `src/` code, `docs/papers/` and `llm/`
docs at `graphify-out/graph.json`. `graphify-out/` is not tracked (excluded via
`.git/info/exclude`, per-clone) — the graph is
**not part of the repo**, so at startup (§0) it must be created (fresh clone)
or updated (stale):

- `graphify-out/graph.json` missing → build it: the full `/graphify` skill
  pipeline (Steps 1-9), or `bin/graphify-refresh` if `graphify-out/cache/`
  exists.
- Graph exists but older than the current code → `bin/graphify-update` first.

**Consult it to understand dependencies before and during work; refresh it after the work is done.** Full guide:
`llm/GRAPHIFY.md`; wrapper scripts: `bin/` (see `bin/README.md`).

Before starting a non-trivial task, map the dependency landscape:

- `bin/graphify-explain "<component>"` — what a component is and what it
  connects to (its callers, callees, shared data)
- `bin/graphify-path "A" "B"` — how two components depend on each other, hop by
  hop; the edge relations show whether the dependency is a call, a shared
  structure, or a paper-documented design link
- `bin/graphify-query "<question>"` — dependency context around a planned change
  (which modules will be affected, which papers document them)
- `bin/graphify-reflect` — prior Q&A feedback (preferred sources, dead ends,
  corrections) before re-deriving known answers

During work, prefer query/path/explain over raw greps for structural questions.
Dirty `graphify-out/` files after an update are expected and are not a reason to
skip the graph. Only skip it when the task is about stale/incorrect graph output
or the user says not to use it.

After all work is done (before reporting done):

1. Run `bin/graphify-update` — AST-only, free, no API key. Re-extracts changed
   code, re-merges the papers/llm graphs, re-clusters, and regenerates
   `GRAPH_REPORT.md` + `graph.html`.
2. If `docs/` or `docs/papers/` changed, run `bin/graphify-refresh` instead —
   it also re-extracts the semantic layer (requires `GEMINI_API_KEY`; without
   it, it falls back to code-only and prints a warning).
3. Report the refresh in the final summary, e.g. "graph updated via
   `bin/graphify-update` → 12,511 nodes, 24,501 edges".

## 8. Build parallelism on a user PC (CNT_CPU_CORE / 2)

Part of the startup checklist (§0): before building anything on a user PC,
compute CNT_CPU_CORE there (physical cores, hyper-threads excluded; E-cores
count) and use `-j<CNT_CPU_CORE / 2>` for **every** build — tests,
performance, and "just check" alike. Never reuse `-j` values from another
machine. How to count and the full rule: `llm/DEVELOPMENT.md` §1 (Build
parallelism).

---

**These guidelines are working if:** fewer unnecessary changes in diffs, fewer rewrites due to overcomplication, clarifying questions come before implementation rather than after mistakes, and work is verified (not assumed) before it is reported done.