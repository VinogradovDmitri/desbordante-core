# PLAN.md — Phase-Based Workflow

Todo files for **every** task — large or small. Huge tasks
(multi-algorithm, large refactors, performance) additionally get
phase decomposition.

## 1. When to use

- A huge task touches several algorithms/files → split into phases,
  one todo file per phase.
- A task changes performance targets (fast paths, threading, bitsets).
- A task needs more than a few commits of coordinated work.
- A small task gets a single `bin/todo_1.md` with granular checkboxes —
  no phases, no extra files.

No "too small" exemption: every task is tracked in a todo file.

## 2. Phases and todo files

- Split into as many phases as reasonable; each is one coherent,
  independently verifiable deliverable.
- **Creating all todo files is always the first task** (`llm/CLAUDE.md`
  §0): `bin/todo_1.md` … `bin/todo_N.md` — one per phase; each contains
  goal, files touched, acceptance criteria, granular checkboxes.
  Follow-up user tasks land in them too. **Before** these, the PREWORK
  check runs: `cp llm/todo_0.md bin/todo_0.md` — verify the environment
  (`llm/PREWORK.sh`); if any check fails, **stop** and ask the user to run
  the relevant PREWORK section; do not proceed to `todo_1.md` until
  `bin/todo_0.md` is fully checked and deleted (`llm/CLAUDE.md` §0
  item 0, §10 S6).
- **Every phase todo transcribes its governing checklist** (S2,
  `llm/CLAUDE.md` §10): item text "§N — <box text>"; statuses per
  S3–S5. **Design/Performance shortcuts:** pre-transcribed templates
  exist (`llm/todo_rules_<num>.md`, `llm/todo_perf_<num>.md`) — copy
  into `bin/` and walk per `llm/CLAUDE.md` §10 S2 (Performance
  templates include the commit-based workflow: baseline → implement
  → measure → commit if faster / undo → delete). The `llm/`
  templates are reused across reviews, never modified — only `bin/`
  copies are live todos.
- Execute phases one by one: finish the todo → run the verification
  pass (`llm/DEVELOPMENT.md` §6) → **conditional loop** on failure
  (fix, re-run from the first failed check) → only then start the
  next phase.
- After a phase is done, **delete its `bin/todo_<num>.md`** (`bin/` is
  untracked, never in the repo).
- **Perf tasks** have fixed todo entries: first = **create the perf
  branch** (`perf/<todo-num>-<short-slug>`, §3) before profiling or
  code change; penultimate = **merge all branches back** into the
  branch where the user called you (perf branch + any `joint/*`,
  conflicts resolved there); last = **targeted tests** (`ctest
  --test-dir build -R "<algo>"` — never the full suite, CI-only per
  `llm/DEVELOPMENT.md` §2) + `valgrind` (memcheck), `helgrind`, `drd`
  — memory errors, data races, deadlocks — before reporting done.

## 3. Performance-target changes → branches

- Any perf task gets its **own branch**:
  `perf/<todo-num>-<short-slug>` (e.g.
  `perf/3-threaded-bitset-build`).
- Creating this branch is the **first task** (§2) — before profiling
  or any code change, so the optimization is tried on an isolated
  experiment branch.
- One branch changes exactly **one type of thing** (one fast path, one
  threading change, one data structure).
- Every perf task records its **numeric expected improvement** in the
  todo file before implementation (e.g. "≥ 5% faster on
  `examples/datasets/sample_height_weight.csv`, measured with the
  §5 protocol").

## 4. Joint validation and the merge decision

- Before merging two perf branches: temporary joint branch
  `joint/<a>-<b>`.
- Measure all four points with the §5 protocol: **base → A → B →
  joint**.
- Merge approved when **either**: **(a)** the joint shows the
  pre-recorded numeric improvement; or **(b)** no measured gain on
  the current processor, but the change improves readability / uses
  less memory / should help other processors, **and** does not
  decrease performance here — then it's a success.
- If the joint is worse than base with no gain and no redeeming
  quality → **deep-think whether the implementation is needed at
  all** — don't merge on autopilot.
- After merging, delete the branch (`git branch -d <branch>` locally,
  and on origin if pushed) and the temporary `joint/*` branches.

## 5. Benchmark measurement protocol

Follow exactly so numbers are comparable. Adapt values to the
machine; the sequence is the point.

```bash
# 1. Stop background update services that add noise
sudo systemctl stop unattended-upgrades
sudo systemctl disable unattended-upgrades

# 2. Drop caches and disable ASLR
sync; echo 3 | sudo tee /proc/sys/vm/drop_caches
echo 0 | sudo tee /sys/kernel/randomize_va_space

# 3. Performance governor on all CPUs
for i in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
do
  echo performance | sudo tee $i
done

# 4. Disable specific CPUs and turbo (cpuX = a real CPU id)
echo 0 | sudo tee /sys/devices/system/cpu/cpuX/online
echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo

# 5. Pin CPU frequency to half of max (non-turbo) — deterministic,
#    machine-independent.
MAX_FREQ=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq)
HALF_FREQ=$((MAX_FREQ / 2))
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies
echo "$HALF_FREQ" | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq
echo "$HALF_FREQ" | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_min_freq
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq   # verify

# 6. Isolate cores 1-4 for the measurement (adjust)
sudo cset shield -c 1-4 -k on

# 7. Measure: 10 repetitions, isolated cores, pinned frequency
cset shield --exec -- perf stat -r 10 <cmd>
```

Notes:
- `scaling_available_frequencies` may be empty (intel_pstate HWP) —
  any `[min, max]` value is then honored.
- Rejected frequency write → pick the listed frequency closest to
  half the max.
- Compare runs only under the same protocol and machine state. Record
  raw numbers in the todo file next to the expected improvement, and
  append them — with the machine state (governor, pinned frequency,
  isolated cores, ASLR off) — to
  `bin/measurements_<YYYY-MM-DD>.md`.
