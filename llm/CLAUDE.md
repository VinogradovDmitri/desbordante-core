# CLAUDE.md

Behavioral guidelines for LLM sessions in this repo. Caution over speed;
use judgment for trivial tasks.

Performance tasks: read `llm/PERFORMANCE.md` (checklist) and `llm/PLAN.md`
§5 (measurement protocol) before touching code.
Review tasks: read `llm/AGENT.md` — reviews run in three modes (Immersion /
Design / Performance, separately or step-by-step); all questions are asked
in an interview **before** the review, none mid-review.

## 0. Startup checklist (any machine, in order)

1. **Todo files** — create all `bin/todo_<num>.md` up front (`llm/PLAN.md`
   §2): one per phase, for **every** task even small ones (small task →
   just `bin/todo_1.md`); follow-up tasks go into them. Perf tasks: first
   todo = create the perf branch; penultimate = merge all branches back
   into the branch where the user called you; last = all tests +
   valgrind/helgrind/drd.
   **Keep todo checkboxes checked in real time**: mark a step `in_progress`
   before doing it and `completed` immediately after — never batch-update
   all boxes at the end. A stale todo list (work done, boxes unchecked) is
   a tracking failure even if the work itself is correct.
   **Todo lists are real files in `bin/todo_<num>.md`** — the in-session
   todo display is only a mirror and must be kept identical to the file;
   never track todos solely in chat/tool state without writing the file.
   **Delete the todo file when all its tasks are done** — a fully checked
   `bin/todo_<num>.md` is removed after the verification pass, per phase
   (`llm/PLAN.md` §2); a leftover completed todo file is also a tracking
   failure.
2. **Session log** — create/append `bin/session_<YYYY-MM-DD>.md`: record
   the task up front, then every command + result as you go. Verified
   decisions logged one per line:
   `Decision: <question> | Chosen: <option> | Confirmed by: user`.
3. **All files live in `bin/` — no exceptions**: todo files, session and
   measurement logs, review reports, and **every temporary/scratch file**
   (profiling data, dumps, notes, one-off scripts, intermediate artifacts).
   Never the opencode dir, repo root, `llm/`, or `graphify-out/`; if a tool
   writes elsewhere, copy/redirect into `bin/`.
4. **Environment bootstrap** (`llm/DEVELOPMENT.md` §0, fresh machine only):
   `.venv`, requirements, uv, clang-format/cmake-format — then verify.
5. **Knowledge graph** (§7): ensure `graphify-out/graph.json` exists and is
   current. Graph tools may be unrunnable (no CLI/API key/long runtime) —
   **ask the user whether they can run it**; if yes, hand them the exact
   command and wait for their call-back; if not, use the existing graph
   and note its vintage when citing.
6. **Build parallelism** (§8): compute CNT_CPU_CORE on this machine; use
   `-j<CNT_CPU_CORE / 2>` for every build (tests, performance, just-check).

## 1. Think before coding
- For any task — even a small one — **ask at least one clarifying question
  first** (intent, scope, success criteria); never silently pick among
  interpretations. **Not certain → ask** — a wrong guess costs more than a
  question.
- State assumptions; **key decisions verified with the user** before acting
  and recorded in the session log. Present multiple interpretations; push
  back when warranted; propose a brief plan for multi-step tasks before
  editing multiple files.

## 2. Simplicity first
Minimum code that solves the problem: no unrequested features, no
single-use abstractions, no speculative flexibility, no error handling for
impossible scenarios. "Would a senior engineer say this is overcomplicated?"

## 3. Surgical changes
- Touch only what the request requires; match existing style; don't improve
  adjacent code; mention unrelated dead code, don't delete it.
- Remove only orphans YOUR changes created.

## 4. Goal-driven execution
- Transform tasks into verifiable goals; per step: `[Step] → verify:
  [check]`.
- Write acceptance criteria into the todo file **before** starting —
  precise and checkable ("`ctest -R <algo>` passes"), never vague; name a
  past example to match its format.
- Before reporting done, run a **second-pass review** (fresh pass or second
  model) against the written criteria; findings fixed or explicitly waived.

## 5. Verify before done
Never declare completion on assumption. Run every applicable check from
`llm/DEVELOPMENT.md`; report each with the exact command + result.
Checks that cannot run locally (clang-tidy, typos — CI-only) are stated as
such, never claimed passed. Say explicitly what you did **not** verify.
Re-read the diff before finishing.

## 6. Definition of done
All of the following:
- [ ] Change is surgical — every changed line traces to the request
- [ ] Code formatted (clang-format v22); CMake formatted if touched
      (`cmake-format --check`)
- [ ] Build passes (same flags as before); targeted tests pass
      (`ctest --test-dir build -R "<algo>"`)
- [ ] Examples + snapshots pass (`pytest ... -k <algo>`; snapshots
      regenerated via the harness and re-verified if output changed)
- [ ] Knowledge graph refreshed — `llm/graphify-update` after code changes
      (§7)
- [ ] Conditional loop respected — on any failed check: fix, then re-run
      from the first failed check; nothing skipped or reported as passed
      without being run
- [ ] Session log updated with commands + results
- [ ] Nothing committed (only when explicitly asked); **nothing pushed —
      `git push` denied in every session, no exceptions (no force-push)**

## 7. Knowledge graph (graphify)
Merged graph of `src/` code, `docs/papers/`, `llm/` docs at
`graphify-out/graph.json`. Not tracked (per-clone `.git/info/exclude`) — a
fresh clone has no graph.

- Missing → build it (full `/graphify` skill pipeline, or
  `llm/graphify-refresh` if `graphify-out/cache/` exists). Stale (code
  newer) → `llm/graphify-update` first.
- **Ask the user before any create/update** (may be unrunnable); if they
  can't, proceed with the existing graph, stating its vintage.
- Before non-trivial work: `llm/graphify-explain "<component>"`,
  `llm/graphify-path "A" "B"`, `llm/graphify-query "<question>"`,
  `llm/graphify-reflect` — prefer these over greps for structural
  questions. Dirty `graphify-out/` after updates is expected; skip the
  graph only for stale-graph tasks or on the user's word.
- After work: `llm/graphify-update` (free, AST-only); if `docs/` or
  `docs/papers/` changed, `llm/graphify-refresh` instead (paper layer needs
  `GEMINI_API_KEY`, else code-only fallback with a warning). Report the
  refresh in the final summary with node/edge counts.
- Graph + Q&A feedback (`graphify-reflect`/`save-result`) + `bin/` session
  logs are the project's **knowledge base** — it improves only if updates
  and saved results actually happen.

## 8. Build parallelism (CNT_CPU_CORE / 2)
Before building on any PC: compute CNT_CPU_CORE there (physical cores only,
hyper-threads excluded, E-cores count; `llm/DEVELOPMENT.md` §1) and use
`-j<CNT_CPU_CORE / 2>` for **every** build — never reuse `-j` from another
machine.

## 9. Autonomy rules — ALWAYS DO / ASK FIRST / NEVER DO
Every action falls into exactly one bucket; unsure → ASK FIRST.

**ALWAYS DO** (no confirmation needed):
- Create `bin/todo_<num>.md` files first, for every task (§0) — real files
  in `bin/`, mirrored in the in-session todo display
- **Update todo statuses in real time** — `in_progress` before a step,
  `completed` right after it; never leave checked-off work unchecked (§0);
  **delete the todo file when all its tasks are done** (§0, `llm/PLAN.md` §2)
- Append every command + result to `bin/session_<YYYY-MM-DD>.md`
- Run the applicable verification chain before reporting done (§5,
  `llm/DEVELOPMENT.md` §6) with the conditional loop
- Consult the knowledge graph before non-trivial work; `llm/graphify-update`
  after (§7)
- Surgical, style-matching edits; clang-format / cmake-format on changed
  files; when a task type repeats, add a fill-in template in
  `llm/templates/`

**ASK FIRST** (destructive, costly, one-way, ambiguous):
- Deleting files or branches, `git reset`/`git rebase`, dropping data,
  `rm` beyond your own scratch files in `bin/`
- Paid API calls (e.g. `graphify-refresh` paper extraction), installing
  anything outside `.venv`, long benchmark runs
- `git commit`, interface/API changes, snapshot regeneration not implied
  by the requested change, expanding the task's scope
- Multiple interpretations or low certainty — asking is mandatory (§1)

**NEVER DO**:
- `git push` — denied in every session, no exceptions (no force-push)
- Hand-editing generated files: example snapshots (regenerate via
  `--snapshot-update`), `llm/requirements.txt` (regenerate via `pip
  freeze`), `graphify-out/*` (regenerated by graphify tools)
- Committing or printing secrets (API keys, tokens)
- Reporting an unrun check as passed

---

**Working if:** fewer unnecessary changes in diffs, fewer rewrites due to
overcomplication, clarifying questions come before implementation rather
than after mistakes, and work is verified (not assumed) before reported
done.
