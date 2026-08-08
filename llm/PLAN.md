# PLAN.md — Phase-Based Workflow

Todo files are created for **every** task — large or small. Huge tasks
(multi-algorithm work, large refactors, performance work) additionally get
phase decomposition.

## 1. When to use

- A huge task touches several algorithms/files at once → split into phases,
  one todo file per phase.
- A task changes performance targets (fast paths, threading, bitsets, …).
- A task needs more than a few commits of coordinated work.
- A small task gets a single `bin/todo_1.md` with granular checkboxes — no
  phases, no extra files.

There is no "too small" exemption: every task is tracked in a todo file.

## 2. Phases and todo files

- Split into as many phases as reasonable; each phase is one coherent,
  independently verifiable deliverable.
- **Creating all todo files is always the first task** (`llm/CLAUDE.md`
  §0): `bin/todo_1.md` … `bin/todo_N.md` — one per phase; a small task has
  only `bin/todo_1.md`. Each contains: goal, files touched, acceptance
  criteria, and as many granular checkboxes as possible. Follow-up user
  tasks land in them too.
- **Every phase todo transcribes its governing checklist** (Superior
  Rule S2, `llm/CLAUDE.md` §10): Design phases carry every box of
  `llm/RULES.md` §1–§19, Performance phases every applicable box of
  `llm/PERFORMANCE.md` §1–§13, verification phases the chain of
  `llm/DEVELOPMENT.md` §6 — item text "§N — <box text>", statuses per
  S3–S5 (§10). A phase whose checklist is not transcribed is not
  started.
- Execute phases one by one: finish the todo (every checkbox) → run the
  verification pass (`llm/DEVELOPMENT.md` §6) and report commands + results
  → **conditional loop** on failure (fix, re-run from the first failed
  check) → only then start the next phase.
- After a phase is done, **delete its `bin/todo_<num>.md`** (`bin/` is
  untracked working state, never in the repo).
- **Performance-related tasks** have two fixed todo entries:
  - The **first task is always "create a new branch for trying to
    implement the current optimization"** (`perf/<todo-num>-<short-slug>`,
    §3) — the branch exists before any profiling or code change.
  - The **penultimate task is merging all new branches back into the
    branch where the user called the session** (the perf branch plus any
    `joint/*` branches, conflicts resolved there).
  - The **last task is always running the targeted tests plus `valgrind`,
    `helgrind`, and `drd`** (`ctest --test-dir build -R "<algo>"` — never
    the full local suite, it is CI-only per `llm/DEVELOPMENT.md` §2;
    + memory errors, data races, deadlocks) before reporting done.

## 3. Performance-target changes → branches

- Any perf task gets its **own branch**: `perf/<todo-num>-<short-slug>`
  (e.g. `perf/3-threaded-bitset-build`).
- Creating this branch is the **first task in the task's todo file** (§2) —
  before profiling or any code change, so the optimization is always tried
  on an isolated experiment branch.
- One branch changes exactly **one type of thing** (one fast path, one
  threading change, one data structure).
- Every perf task records its **numeric expected improvement** in the todo
  file before implementation (e.g. "≥ 5% faster on
  `examples/datasets/sample_height_weight.csv`, measured with the §5
  protocol").

## 4. Joint validation and the merge decision

- Before merging two perf branches: temporary joint branch
  `joint/<a>-<b>`.
- Measure all four points with the §5 protocol: **base → branch A →
  branch B → joint**.
- Merge approved when **either**: **(a)** the joint shows the pre-recorded
  numeric improvement; or **(b)** no measured gain on the current
  processor, but the change improves readability / consumes less memory /
  should help other processors, **and** does not decrease performance on
  the current processor — then the implementation is a success.
- If the joint is worse than base with no gain and no redeeming quality
  (readability / memory / other-hardware potential) → **deep-think whether
  the implementation is needed at all** — don't merge on autopilot.
- After merging, delete the branch (`git branch -d <branch>` locally, and
  on origin if it was pushed) and the temporary `joint/*` branches.

## 5. Benchmark measurement protocol

Follow exactly so numbers are comparable. Adapt the commented values to the
machine; the sequence is the point.

```bash
# 1. Stop background update services that add noise
sudo systemctl stop unattended-upgrades
sudo systemctl disable unattended-upgrades

# 2. Drop caches and disable ASLR
sync; echo 3 | sudo tee /proc/sys/vm/drop_caches
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space

# 3. Performance governor on all CPUs
for i in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
do
  echo performance | sudo tee $i
done

# 4. Disable specific CPUs and turbo (cpuX = a real CPU id on this machine)
echo 0 | sudo tee /sys/devices/system/cpu/cpuX/online
echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo

# 5. Pin the CPU frequency to half of the maximum (non-turbo) frequency.
#    Half-of-max is deterministic and machine-independent.
MAX_FREQ=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq)  # after turbo is off
HALF_FREQ=$((MAX_FREQ / 2))
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies
echo "$HALF_FREQ" | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq
echo "$HALF_FREQ" | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_min_freq

cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq   # to verify

# 6. Isolate cores 1-4 for the measurement (adjust the set)
sudo cset shield -c 1-4 -k on

# 7. Measure: 10 repetitions, isolated cores, pinned frequency
cset shield --exec -- perf stat -r 10 <cmd>
```

Notes:
- `scaling_available_frequencies` may be empty (intel_pstate HWP) —
  frequencies are then continuous, any `[min, max]` value is honored.
- Rejected frequency write → pick the listed frequency closest to half the
  max.
- Compare runs only when both were taken under the same protocol and
  machine state. Record raw numbers in the todo file next to the expected
  improvement, and append them — together with the machine state (governor,
  pinned frequency, isolated cores, ASLR off) — to
  `bin/measurements_<YYYY-MM-DD>.md`.
