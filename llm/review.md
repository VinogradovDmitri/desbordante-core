# Review Preparation

`make review` is the common preparation command for algorithm reviews. It
does not perform the review and it never creates commits or patches. It makes
the target, delivery contract, phase plan, and next-session context explicit
before an LLM spends tokens reading the repository.

## Quick Start

```bash
make review
```

The default target is the current branch's configured upstream versus `HEAD`.
For a branch without an upstream, provide a base revision explicitly:

```bash
make review BASE=10677499 HOURS=30 MODE=patches
```

Useful variables:

| Variable | Default | Meaning |
|---|---|---|
| `MODE` | interactive, otherwise `report` | `commits`, `patches`, or `report` |
| `BASE` | current branch upstream | Review base revision |
| `HEAD` | `HEAD` | Review head revision |
| `HOURS` | `3` | Estimated total review effort |
| `PHASES` | derived | Explicit minimum phase count |
| `REVIEW_MODES` | `immersion,design,performance` | Review modes to prepare (comma-separated subset) |
| `VERIFY` | `build` | Verification checks to run (comma-separated subset of `build`,`tests`,`python`,`snapshots`,`determinism`,`profiling`) |
| `FORCE` | empty | `FORCE=1` regenerates the phase files and brief, replacing existing numeric phase files |
| `OUTPUT_DIR` | `bin` | Brief and live phase todo directory |
| `REPORT` | `bin/report.md` | Report path recorded in the brief |
| `PATCH_DIR` | `bin/patches` | Patch directory recorded in the brief |

`make review HOURS=30` creates at least 15 phase files, not three. The
default phase target is approximately two hours and must remain within the
1–3 hour range. `PHASES` can raise the count when the user already knows the
task needs more boundaries.

## Output Contracts

These are delivery contracts, separate from the Immersion/Design/Performance
review modes:

1. `commits`: implement each accepted finding as one focused commit in a
   worktree under `bin/`, then map each commit to `report.md`.
2. `patches`: implement each accepted finding as one stable numbered patch,
   then map each patch to `report.md` and verify the complete series.
3. `report`: make no code changes. Write findings and suggested fixes to
   `report.md`; every suggested fix belongs in a fenced code block.

The preparation command only records this contract. The LLM performs the
selected delivery after reviewing and verifying each finding.

## Examples

All modes write `bin/session_brief.md` and one `bin/todo_<num>.md` per planned
phase; the delivery differs afterwards:

```bash
# commits: one focused commit per accepted finding in a bin/ worktree,
# each mapped to an entry in bin/report.md
make review MODE=commits HOURS=12

# patches: one numbered bin/patches/<NN>_<finding>.patch per accepted
# finding, series verified, entries mapped to bin/report.md
make review BASE=10677499 MODE=patches HOURS=30

# report only: findings plus fenced-code suggested fixes in bin/report.md,
# no commits or patches are created
make review MODE=report HOURS=6
```

## Troubleshooting

| Symptom | Cause and fix |
|---|---|
| `No configured upstream for the current branch` | The branch tracks no remote; rerun with `make review BASE=<revision>` or configure a tracking branch. |
| `Cannot resolve base revision ...` | The revision does not exist; pass a commit, tag, or ref that `git rev-parse` accepts. |
| `Invalid review output ...` / `Invalid REVIEW_MODES ...` / `HOURS must be a positive number` | Wrong variable value; see the table above for allowed values. |
| `python3: not found` | The interpreter is missing; use the repository venv: `make review PYTHON=.venv/bin/python3`. |
| `Active phase todos already exist ...` | An interrupted session left live phase files; continue them, or regenerate deliberately with `make review FORCE=1`. |
| Dirty working tree | Uncommitted files are excluded from the revision diff and noted in the brief `Warnings`; they are not review input. |

## Generated Files

The command writes:

- `bin/session_brief.md`: compact target, mode, footprint, phase, command,
  warning, and resume context.
- `bin/todo_1.md` … `bin/todo_N.md`: one live file per planned phase.

Each phase contains a goal, a governing checklist, an ordered phase
checklist, and exit criteria. The first phase is marked `in_progress`; later
phases are pending. Existing numeric phase files are never overwritten unless
`FORCE=1` is passed deliberately (or `--force` to `llm/review_prepare.py`).

### Governing checklist generation

Do not maintain generated todo templates under `llm/`.

- Design review phases transcribe every checkbox from `llm/RULES.md` §1–§19.
- Performance review phases transcribe every checkbox from
  `llm/PERFORMANCE.md` §1–§13.
- Build/test phases cite `llm/DEVELOPMENT.md` §6 and include the selected
  targeted verification commands.

`llm/RULES.md`, `llm/PERFORMANCE.md`, and `llm/DEVELOPMENT.md` remain the
authoritative sources. Generated checklists live under `bin/`, are updated in
real time, and are deleted only after their verification pass is complete.

## Interview And Gates

Before verdicts, the LLM must interview the user about review modes, target,
output contract, verification, performance datasets, and estimated effort.
Both review modes and verification are multi-select: the user may pick any
comma-separated subset (for example `design,performance` or `build,tests`).
`make review` prompts for each when run interactively and accepts multiple
choices; non-interactive runs use `REVIEW_MODES=` and `VERIFY=`. Performance
and benchmark work checks PREWORK sections B/C only after the interview
selects them. A review-only preparation does not require profiling tools.

The command reviews committed revisions, not uncommitted working-tree files.
It records a dirty-tree warning in the brief so unrelated local changes are
not silently treated as review input.

## Resuming

At the next session:

1. Read `bin/session_brief.md` and the latest `bin/session_<date>.md`.
2. Keep incomplete phase files and continue the first `in_progress` phase.
3. Update the phase file immediately after each verified command.
4. Create additional phases if scope expands; do not stretch a phase past
   three hours.
5. Delete a phase file only after all boxes are verified and the result is in
   the session log/report.

The startup PREWORK gate and the review interview still apply; this command
does not bypass them.
