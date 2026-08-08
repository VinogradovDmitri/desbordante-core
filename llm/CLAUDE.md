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
   into the branch where the user called you; last = **targeted tests**
   (ctest `-R "<algo>"`, never the full suite — CI-only) plus
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
   writes elsewhere, copy/redirect into `bin/`. Worktrees go in `bin/` too
   (`git worktree add bin/<name> <branch>`), and **once the last commit on
   that branch is done, the worktree is removed so the branch is free for
   the user** (`git worktree remove --force bin/<name>`; the commit is safe
   in git, only the local build cache is lost).
4. **Environment bootstrap** (`llm/DEVELOPMENT.md` §0, fresh machine only):
   `.venv`, requirements, uv, clang-format/cmake-format — then verify.
5. **Knowledge graph** (§7): ensure `graphify-out/graph.json` exists and is
   current; if it is missing or has not been updated for a long time, write
   a warning to the user (§7) — never build or refresh it yourself.
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
- [ ] Verification chain (`llm/DEVELOPMENT.md` §6) run with the
      conditional loop (§5) — build, targeted tests, examples/snapshots
- [ ] Session log updated with commands + results (§0)
- [ ] Nothing committed (only when explicitly asked) or pushed (§9)

## 7. Knowledge graph (graphify)
Merged graph of `src/` code, `docs/papers/`, `llm/` docs at
`graphify-out/graph.json`. Not tracked (per-clone `.git/info/exclude`) — a
fresh clone has no graph.

- The LLM **never creates or updates the graph** — building/refreshing is
  the user's job (`llm/README.md`, `llm/GRAPHIFY.md`). If the graph is
  missing or stale (code/docs newer than the graph — check with
  `find src docs llm -newer graphify-out/graph.json -print -quit`, or the
  graph has not been updated for a long time), **write a warning to the
  user** and continue with the existing graph, stating its vintage.
- Before non-trivial work: `llm/graphify-explain "<component>"`,
  `llm/graphify-path "A" "B"`, `llm/graphify-query "<question>"`,
  `llm/graphify-reflect` — prefer these over greps for structural
  questions. Skip the graph only on the user's word.
- Graph + Q&A feedback (`graphify-reflect`/`save-result`) + `bin/` session
  logs are the project's **knowledge base** — it improves only if the user
  runs updates and saved results actually happen.

## 8. Build parallelism (CNT_CPU_CORE / 2)
`-j<CNT_CPU_CORE / 2>`, computed on the build machine (§0 item 6); counting
method and pitfalls: `llm/DEVELOPMENT.md` §1.

## 9. Autonomy rules — ALWAYS DO / ASK FIRST / NEVER DO
Every action falls into exactly one bucket; unsure → ASK FIRST.

**ALWAYS DO** (no confirmation needed):
- Create `bin/todo_<num>.md` files first, for every task (§0 item 1) —
  real files in `bin/`, mirrored in the in-session todo display
- **Update todo statuses in real time** — `in_progress` before a step,
  `completed` right after it (§0 item 1, §10 S3); **delete the todo file
  when all its tasks are done** (§0 item 1, `llm/PLAN.md` §2)
- Append every command + result to `bin/session_<YYYY-MM-DD>.md`
  (§0 item 2)
- Run the applicable verification chain before reporting done (§5,
  `llm/DEVELOPMENT.md` §6) with the conditional loop
- Consult the knowledge graph before non-trivial work (§7); warn the user
  if it is missing or stale
- Surgical, style-matching edits; clang-format / cmake-format on changed
  files; when a task type repeats, add a fill-in template in
  `llm/templates/`
- Commit messages (only when a commit is explicitly asked): single-line
  subject, no description — the repo convention (all history is one-liners)

**ASK FIRST** (destructive, costly, one-way, ambiguous):
- Deleting files or branches, `git reset`/`git rebase`, dropping data,
  `rm` beyond your own scratch files in `bin/`
- Paid API calls, installing
  anything outside `.venv`, long benchmark runs
- `git commit`, interface/API changes, snapshot regeneration not implied
  by the requested change, expanding the task's scope; multi-line commit
  messages (description) — repo convention is single-line subjects
- Multiple interpretations or low certainty — asking is mandatory (§1)

**NEVER DO**:
- `git push` — denied in every session, no exceptions (no force-push)
- Hand-editing generated files: example snapshots (regenerate via
  `--snapshot-update`), `llm/requirements.txt` (regenerate via `pip
  freeze`), `graphify-out/*` (regenerated by graphify tools)
- Committing or printing secrets (API keys, tokens)
- Reporting an unrun check as passed

## 10. Superior rules — todo & checklist discipline (S1–S6)

Mandatory in every session. These exist because a multi-phase review
was once tracked in a single stale todo list with batch-checked boxes
and a skipped checklist walk — never again. A short mandatory version
is in the root `CLAUDE.md` (always in context); this section is the
authoritative full text.

**S1 — Todo files first, one per phase.** The first action of any
task creates all `bin/todo_<num>.md` files — one per phase, before any
exploration or tool use. Multi-phase tasks (reviews: one file per mode
Immersion/Design/Performance plus verification and report; perf tasks:
per `llm/PLAN.md` §2/§3) MUST have multiple files. A single
`bin/todo_1.md` is allowed only for genuinely small single-phase tasks.

**S2 — Transcribe the governing checklist into the todo.** Before a
phase starts, its governing checklist is transcribed into its todo
file as granular checkboxes: Design → every box of `llm/RULES.md`
§1–§19; Performance → every applicable box of `llm/PERFORMANCE.md`
§1–§13; build/tests → the verification chain of
`llm/DEVELOPMENT.md` §6. Item text = "§N — <box text>" so the report
can cite it. A phase whose checklist is not transcribed is not started.

**S3 — Real-time statuses, never batch.** Mark a box `in_progress`
before starting it and `completed` immediately after finishing it.
Never batch-update boxes at the end of a phase; never leave finished
work unchecked. The file `bin/todo_<num>.md` and the in-session todo
display are mirrors — update both in the same message.

**S4 — Walk checklists step by step, in order.** `RULES.md` §1→§19 and
`PERFORMANCE.md` §1→§13 are processed top to bottom; every box gets a
verdict (Pass / Fail / N/A / Needs-info). No skipping, no
cherry-picking, no "I covered that informally".

**S5 — Completed = verified.** A box is `completed` only after its
verification actually ran (command executed, log inspected, result
recorded in the session log). Never "completed by assumption".

**S6 — Context guarantee / startup gate.** The root `CLAUDE.md` is
auto-loaded every session; it mandates, before ANY tool call:
1. `ls bin/todo_*.md` — per-phase files exist (S1)?
2. read the tail of the latest `bin/session_<YYYY-MM-DD>.md` — state
   restored?
3. missing files → create immediately, transcribing the governing
   checklist (S2);
4. statuses live (S3), checklists in order (S4), verified before
   completed (S5).
A session that starts without this gate has failed its first rule.

---

**Working if:** fewer unnecessary changes in diffs, fewer rewrites due to
overcomplication, clarifying questions come before implementation rather
than after mistakes, and work is verified (not assumed) before reported
done.
