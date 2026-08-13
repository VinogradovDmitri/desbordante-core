---
name: algorithm-review
description: Use when the user asks to review an algorithm contribution, run `make review`, or produce a review report (Immersion/Design/Performance, modes commits/patches/report) in the Desbordante repo. Runs the whole review lifecycle — prep, interview, checklist verdicts, report.md with per-issue template — from the llm/ guidance files, without re-asking what `make review` already recorded.
---

# Algorithm Review (Desbordante)

Review an algorithm/primitive contribution end-to-end. The full how-to lives
in `llm/AGENT.md` (authoritative); requirements in `llm/RULES.md`;
preparation in `llm/review.md`; commands in `llm/DEVELOPMENT.md`; perf
checklist in `llm/PERFORMANCE.md`. Read `llm/AGENT.md` first, then follow it.
This skill is the entry point so the user does not re-explain the review.

## When this triggers

The user asks to review an algorithm (FD, UCC, DC, AR, MD, ND, RFD, OD, IND,
graph patterns, …) or says `make review` / "review" / "report.md". The skill
replaces the need to run `make review` in a separate terminal and then repeat
the setup in OpenCode — everything happens in one session.

## Workflow (in order)

1. **Read state.** Read `bin/session_brief.md` if it exists — it records
   mode(s), target, output contract, verification, effort, and phase plan
   collected by `make review`. Also read the tail of the latest
   `bin/session_<YYYY-MM-DD>.md` to restore session state. Never re-ask
   anything already recorded there.
2. **Prepare** (if the brief is missing or stale — ask the user only for
   unrecorded values): run `make review` with the user's chosen variables
   (`MODE=commits|patches|report`, `BASE=`, `HEAD=`, `HOURS=`, `PHASES=`,
   `REVIEW_MODES=`, `VERIFY=`, `FORCE=1`). It writes `bin/session_brief.md`
   plus one `bin/todo_<num>.md` per phase. If the user already ran it, just
   read the files — do not re-prepare.
3. **PREWORK gate (S6).** `mkdir -p bin && cp llm/todo_0.md bin/todo_0.md`.
   Check section A now (always). Check sections B/C only after the interview
   answers select Performance (B) or benchmark (C). If a check fails, stop
   and ask the user to run that `llm/PREWORK.sh` section. Delete
   `bin/todo_0.md` only when all applicable checks pass.
4. **Interview** — only what `make review` does not collect:
   algorithm-text source (if Immersion: pasted / `docs/papers/…` / none —
   none ⇒ Immersion passes), measurement dataset (if Performance), scope,
   known issues, other notes. Track it in a temporary `bin/todo_tmp_<num>.md`
   and delete it when done. **No questions mid-review** — ambiguity during
   the review is marked **Needs-info** and listed in "Unresolved items".
5. **Todos.** Create all `bin/todo_<num>.md` from the brief; Design phases
   transcribe `llm/RULES.md` §1–§19, Performance phases transcribe
   `llm/PERFORMANCE.md` §1–§13, build/test phases cite `llm/DEVELOPMENT.md`
   §6. Update statuses in real time; walk checklists top to bottom; delete
   each phase file only after every box is verified.
6. **Execute the modes** in order (only those selected):
   - **Immersion**: provided algorithm text vs implementation — requirements
     coverage + architecture; no text ⇒ pass and say so.
   - **Design**: `llm/RULES.md` §1–§19 top to bottom, every checkbox
     (Pass/Fail/N/A/Needs-info with `file:line` and how verified). Read
     `src/tests/unit/test_<algo>.cpp` before verdicts; check cross-algorithm
     precedent via `llm/graphify-explain` / `llm/graphify-path`.
   - **Performance**: `llm/PERFORMANCE.md` §1–§13, footprint only; determinism
     probe (30-process) and `valgrind --tool=memcheck`, `helgrind`, `drd`
     required — any error is **Blocking**. Profile suspected hot spots before
     reporting.
7. **Write `bin/report.md`** — the deliverable for all output contracts.
   Append each issue as found, in discovery order, **no sorting**, using the
   per-issue template in `llm/AGENT.md` → "`report.md` per-issue template":
   name + type (Blocking/Major/Minor/nit), files & line ranges, why it is an
   issue with **navigable proof** (violated `RULES.md` checkbox, tool log
   `bin/*.log` with line numbers, measurement file with lines, or quoted
   Immersion text vs `file:line`), suggested fix per output mode (`report` =
   fenced code block, `commits` = commit hash, `patches` = patch path), and
   perf impact (measurement file + lines + gain) for performance issues.
   Never write "must" without explaining why.
8. **Deliver per output contract:**
   - `report`: no code changes — fixes live in fenced code blocks in
     `report.md`.
   - `commits`: fix each accepted finding as one focused commit in a worktree
     under `bin/` (`git worktree add bin/<name> <branch>`; datasets symlink
     recipe in `llm/DEVELOPMENT.md` §1), map each commit to a report entry,
     propose squashing, free the worktree.
   - `patches`: one stable numbered `bin/patches/<NN>-<name>.patch` per
     accepted finding, verify the complete series, map to report entries.
9. **Verify, don't assume.** Run the verification selected in the brief
   (build / tests / python / snapshots / determinism / profiling) with
   `-j<CNT_CPU_CORE / 2>`; state how each verdict was verified. Never build
   in `/tmp`. Log every command + result to `bin/session_<YYYY-MM-DD>.md`.
10. **Second pass** before finishing: re-verify every Fail (false positives),
    calibrate severity, check coverage (every footprint file, every RULES.md
    section, every verification step has a result).

## Rules

- Read `llm/AGENT.md` at the start of the review and follow it — this skill
  summarizes, it does not override.
- Never re-ask what `bin/session_brief.md` records; never `git push`;
  never hand-edit generated snapshots (`--snapshot-update` instead).
- The report's issues are not sorted by severity — append new findings
  immediately in discovery order.
- Every report entry must be self-navigable: the user finds the proof by
  reading the description alone (exact file, line range, or quoted text).
