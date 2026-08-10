# CLAUDE.md

Behavioral guidelines for LLM sessions in this repo. Caution over
speed; use judgment for trivial tasks.

Performance tasks: read `llm/PERFORMANCE.md` (checklist) and
`llm/PLAN.md` §5 (measurement protocol) before touching code.
Review tasks: read `llm/AGENT.md` — reviews run in three modes
(Immersion / Design / Performance), separately or step-by-step; all
questions in an interview **before** the review, none mid-review.

## 0. Startup checklist (any machine, in order)

0. **PREWORK check** — `mkdir -p bin && cp llm/todo_0.md bin/todo_0.md`,
   check every applicable box. If any check **fails**, **stop** and ask
   the user to run the relevant `llm/PREWORK.sh` section (sudo/install commands
   the LLM cannot run); do not proceed to `bin/todo_1.md` until
   `bin/todo_0.md` is fully checked and deleted.
1. **Todo files** — create all `bin/todo_<num>.md` up front
   (`llm/PLAN.md` §2): one per phase, for **every** task (small →
   just `bin/todo_1.md`); follow-ups go into them. Perf tasks: first
   = create the perf branch; penultimate = merge all branches back;
   last = **targeted tests** (`ctest -R "<algo>"`, never the full
   suite — CI-only) + valgrind/helgrind/drd.
   **Real-time checkboxes**: `in_progress` before a step, `completed`
   right after — never batch at the end (a stale todo list is a
   tracking failure). **Real files in `bin/todo_<num>.md`** — the
   in-session display mirrors the file; never track solely in chat
   state. **Delete when all done** — a fully checked file is removed
   after the verification pass; a leftover completed file is also a
   tracking failure.
2. **Session log** — create/append `bin/session_<YYYY-MM-DD>.md`:
   task up front, then every command + result. Decisions logged as
   `Decision: <q> | Chosen: <opt> | Confirmed by: user`.
3. **All files in `bin/`** — todo, session/measurement logs, review
   reports, **every scratch file** (profiling, dumps, notes, scripts,
   artifacts). Never the opencode dir, repo root, `llm/`, or
   `graphify-out/`; redirect into `bin/` if a tool writes elsewhere.
   Worktrees go in `bin/` too, and **once the last commit on that
   branch is done, the worktree is removed** (`git worktree remove
   --force bin/<name>`) so the branch is free for the user.
4. **Environment bootstrap** (`llm/DEVELOPMENT.md` §0, fresh machine):
   `.venv`, requirements, uv, clang-format/cmake-format — then verify.
5. **Knowledge graph** (§7): ensure `graphify-out/graph.json` exists
   and is current; if missing/stale, **warn the user** (§7) — never
   build or refresh it yourself.
6. **Build parallelism** (§8): `CNT_CPU_CORE` on this machine; use
   `-j<CNT_CPU_CORE / 2>` for every build.

## 1. Think before coding
- **Ask at least one clarifying question first** (intent, scope,
  success criteria); never silently pick among interpretations.
  **Not certain → ask** — a wrong guess costs more than a question.
- State assumptions; **key decisions verified with the user** before
  acting and recorded in the session log. Present multiple
  interpretations; push back when warranted; propose a brief plan for
  multi-step tasks before editing multiple files.

## 2. Simplicity first
Minimum code that solves the problem: no unrequested features, no
single-use abstractions, no speculative flexibility, no error handling
for impossible scenarios. "Would a senior engineer say this is
overcomplicated?"

## 3. Surgical changes
- Touch only what the request requires; match existing style; don't
  improve adjacent code; mention unrelated dead code, don't delete it.
- Remove only orphans YOUR changes created.

## 4. Goal-driven execution
- `[Step] → verify: [check]`.
- Write acceptance criteria into the todo file **before** starting —
  precise and checkable ("`ctest -R <algo>` passes"), never vague.
- Before reporting done, run a **second-pass review** (fresh pass or
  second model) against the written criteria; findings fixed or
  waived.

## 5. Verify before done
Never declare completion on assumption. Run every applicable check
from `llm/DEVELOPMENT.md`; report each with exact command + result.
CI-only checks (clang-tidy, typos) are stated as such, never claimed
passed. Say what you did **not** verify. Re-read the diff before
finishing.

## 6. Definition of done
- [ ] Change is surgical — every changed line traces to the request
- [ ] Code formatted (clang-format v22); CMake formatted if touched
      (`cmake-format --check`)
- [ ] Verification chain (`llm/DEVELOPMENT.md` §6) run with the
      conditional loop (§5) — build, targeted tests, examples/snapshots
- [ ] Session log updated with commands + results (§0)
- [ ] Nothing committed (only when explicitly asked) or pushed (§9)

## 7. Knowledge graph (graphify)
Merged graph of `src/`, `docs/papers/`, `llm/` at
`graphify-out/graph.json`. Not tracked (per-clone `.git/info/exclude`).
- **Never create or update the graph** — building/refreshing is the
  user's job (`llm/README.md`, `llm/GRAPHIFY.md`). If missing/stale
  (check: `find src docs llm -newer graphify-out/graph.json -print
  -quit`), **warn the user** and continue with the existing graph,
  stating its vintage.
- Before non-trivial work: `llm/graphify-explain "<component>"`,
  `llm/graphify-path "A" "B"`, `llm/graphify-query "<question>"`,
  `llm/graphify-reflect` — prefer these over greps for structural
  questions. Skip only on the user's word.
- Graph + Q&A feedback + `bin/` session logs are the project's
  **knowledge base** — it improves only if the user runs updates and
  saved results actually happen.

## 8. Build parallelism (CNT_CPU_CORE / 2)
`-j<CNT_CPU_CORE / 2>`, computed on the build machine (§0 item 6);
counting method and pitfalls: `llm/DEVELOPMENT.md` §1.

## 9. Autonomy rules — ALWAYS DO / ASK FIRST / NEVER DO
Unsure → ASK FIRST.

**ALWAYS DO**: create `bin/todo_<num>.md` first, for every task (§0
item 1); **update statuses in real time** — `in_progress` before,
`completed` after (§0, §10 S3); **delete when all done** (§0,
`llm/PLAN.md` §2). Append every command + result to
`bin/session_<YYYY-MM-DD>.md` (§0 item 2). Run the verification chain
before reporting done (§5, `llm/DEVELOPMENT.md` §6). Consult the
knowledge graph before non-trivial work (§7); warn if missing/stale.
Surgical, style-matching edits; clang-format / cmake-format on changed
files; add a fill-in template in `llm/templates/` when a task type
repeats. Commit messages (only when explicitly asked): single-line
subject, no description (repo convention).

**ASK FIRST** (destructive, costly, one-way, ambiguous): deleting
files/branches, `git reset`/`git rebase`, dropping data, `rm` beyond
your own scratch files; paid API calls, installing anything outside
`.venv`, long benchmark runs; `git commit`, interface/API changes,
snapshot regeneration not implied by the change, scope expansion,
multi-line commit messages; multiple interpretations or low certainty
(§1).

**NEVER DO**: `git push` — denied in every session, no exceptions
(no force-push). Hand-editing generated files (example snapshots →
`--snapshot-update`; `llm/requirements.txt` → `pip freeze`;
`graphify-out/*` → graphify tools). Committing or printing secrets.
Reporting an unrun check as passed.

## 10. Superior rules — todo & checklist discipline (S1–S6)

Mandatory in every session. Brief version in root `CLAUDE.md`
(always in context); this is the authoritative full text.

**S1 — Todo files first, one per phase.** Create all
`bin/todo_<num>.md` — one per phase — before any exploration or tool
use. Multi-phase tasks (reviews: one per mode + verification + report;
perf: per `llm/PLAN.md` §2/§3) MUST have multiple files. A single
`bin/todo_1.md` only for genuinely small single-phase tasks.

**S2 — Transcribe the governing checklist into the todo.** Before a
phase starts, transcribe its governing checklist as granular
checkboxes prefixed `§N — <box text>` so the report can cite it:
Design → `llm/RULES.md` §1–§19; Performance →
`llm/PERFORMANCE.md` §1–§13; build/tests → `llm/DEVELOPMENT.md` §6.
A phase whose checklist is not transcribed is not started.
**Pre-transcribed template shortcuts** (the transcription is
pre-done; copy the needed templates into `bin/` and walk each `bin/`
copy as the phase's todo — real-time statuses per S3, in order per
S4, delete the `bin/` copy once all its boxes are `completed` per S1;
the `llm/` templates are never modified — only `bin/` copies are
live todos):
- **Design:** `cp llm/todo_rules_<num>.md bin/todo_rules_<num>.md`
  (§1–§19 or only applicable sections); walk in §1→§19 order.
- **Performance:** `cp llm/todo_perf_<num>.md bin/todo_perf_<num>.md`
  (§1–§13; §3 and §11 are prose-only → single read-and-confirm
  checkbox); walk in §1→§13 order. Each Performance template
  includes a **"Workflow" section** driving a commit-per-section
  cycle: baseline (quick, not a long benchmark) → implement (walk
  the checklist) → measure → **commit only if faster** (undo if not
  — revert so the working tree returns to baseline) → delete
  `bin/todo_perf_<num>.md`.

**S3 — Real-time statuses, never batch.** `in_progress` before
starting, `completed` immediately after. Never batch at the end; the
file and the in-session display are mirrors — update both in the
same message.

**S4 — Walk checklists in order.** `RULES.md` §1→§19 and
`PERFORMANCE.md` §1→§13 are processed top to bottom; every box gets a
verdict (Pass / Fail / N/A / Needs-info). No skipping, no
cherry-picking.

**S5 — Completed = verified.** A box is `completed` only after its
verification actually ran (command executed, log inspected, result
recorded in the session log). Never "completed by assumption".

**S6 — Startup gate.** Before ANY tool call: **(0)** `mkdir -p bin && cp
llm/todo_0.md bin/todo_0.md` — PREWORK environment check; if any box
fails, **stop** and ask the user to run the relevant `llm/PREWORK.sh`
section (`llm/CLAUDE.md` §0 item 0) — do not proceed until
`bin/todo_0.md` is fully checked and deleted; (1) `ls bin/todo_*.md` —
per-phase files exist (S1)? (2) read the tail of the latest
`bin/session_<YYYY-MM-DD>.md` — state restored? (3) missing → create
immediately, transcribing the governing checklist (S2); (4) statuses
live (S3), checklists in order (S4), verified before completed (S5).
A session that starts without this gate has failed its first rule.

---

**Working if:** fewer unnecessary changes in diffs, fewer rewrites due
to overcomplication, clarifying questions before implementation, and
work verified (not assumed) before reported done.
