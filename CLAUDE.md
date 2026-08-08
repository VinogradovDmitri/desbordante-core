# CLAUDE.md

The full project guidance lives in `llm/` — read these when working in this repo:

- `llm/CLAUDE.md` — general assistant guidance
- `llm/AGENT.md` — how to review algorithm contributions
- `llm/RULES.md` — the checkable development requirements
- `llm/DEVELOPMENT.md` — build, test, snapshot, and format commands
- `llm/PLAN.md` — phase/todo workflow for huge tasks, perf branches, measurement protocol
- `llm/PERFORMANCE.md` — performance optimization checklist for data-profiling algorithms
- `llm/GRAPHIFY.md` — knowledge graph usage guide (`graphify-out/`, `llm/` wrapper scripts)
- `llm/templates/` — fill-in task templates (review, implement, optimize performance)

## Superior Rules — always in context (S1–S6)

Todo & checklist discipline is governed by the Superior Rules S1–S6
(full text: `llm/CLAUDE.md` §10). They are mandatory in every session —
including review sessions.

**Startup gate (S6) — run BEFORE any tool call:**
1. `ls bin/todo_*.md` — one todo file per phase must exist (S1)
2. read the tail of the latest `bin/session_<YYYY-MM-DD>.md` — restore state
3. missing per-phase todo files → create immediately, transcribing the
   governing checklist (RULES.md / PERFORMANCE.md / DEVELOPMENT.md §6)
   into them as granular checkboxes (S2)
4. statuses updated in real time, never batch-checked (S3); checklists
   walked top-to-bottom in order (S4); a box is `completed` only after
   its verification actually ran (S5)

A session that starts without this gate has failed its first rule.

**Build-environment pitfalls (verified — see `llm/DEVELOPMENT.md` §1):**
never build in `/tmp` (small tmpfs here → disk-full, cascades into fake
`as: BFD assertion fail` errors); when building from a git worktree,
replace the tracked `datasets/` dir with a symlink via `rm -rf` + `ln -s`
(plain `ln -s` creates a nested symlink and configure fails on
`datasets.zip`); after the last commit on a worktree branch, free it
(`git worktree remove --force bin/<name>`) — a worktree-checked-out
branch is locked and the user cannot `git switch` to it.
