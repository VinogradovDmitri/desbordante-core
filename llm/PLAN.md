# PLAN.md — Phase-Based Workflow

How to decompose, track, and merge tasks. Todo files are created for **every**
task — large or small. Huge tasks (multi-algorithm work, large refactors,
performance work) additionally get phase decomposition.

## 1. When to use

Todo files (`bin/todo_<num>.md`) are created for **all tasks**, even small
ones:

- A huge task touches several algorithms, files, or subsystems at once — split
  it into phases, one todo file per phase.
- A task changes performance targets (fast paths, threading, bitsets, …).
- A task needs more than a few commits of coordinated work.
- A small task gets a single `bin/todo_1.md` with its granular checkboxes — no
  phases, no extra files.

There is no "too small" exemption: every task is tracked in a todo file.

## 2. Phases and todo files

- Split a huge task into **as many phases as reasonable**. Each phase is one
  coherent deliverable that can be verified independently. A small task has a
  single phase (one todo file).
- **Creating all todo files is always the first task** (see `llm/CLAUDE.md`
  §0). The LLM generates **all** phase files up front:
  - `bin/todo_1.md`, `bin/todo_2.md`, …, `bin/todo_N.md` — one numbered file
    per phase; a small task has only `bin/todo_1.md`.
  - Every todo file contains: goal, files touched, acceptance criteria, and
    **as many granular checkbox tasks as possible** (each task = one small,
    individually verifiable step).
- Follow-up tasks from the user land in the todo files too — e.g. the next
  tasks go into `bin/todo_1.md`.
- Execute phases **one by one**:
  1. Finish `bin/todo_<num>.md` — every checkbox checked.
  2. Run the verification pass from `llm/DEVELOPMENT.md` §6 (build, targeted
     `ctest -R`, example `pytest`, clang-format, cmake-format) and report the
     commands + results.
  3. Only then start `bin/todo_<num+1>.md`.
- After a phase is successfully implemented, **delete its `bin/todo_<num>.md`**
  file. `bin/` is untracked (see `.git/info/exclude`), so todo files never enter the
  repo and are purely working-tool state.

## 3. Performance-target changes → branches

- Any task that changes performance targets gets its **own branch**:
  `perf/<todo-num>-<short-slug>`, e.g. `perf/3-threaded-bitset-build`.
- One branch changes exactly **one type of thing** (one fast path, one
  threading change, one data structure).
- Every performance task records its **numeric expected improvement** in the
  todo file before implementation, e.g.:
  "≥ 5% faster on `examples/datasets/sample_height_weight.csv`, measured with
  the protocol in PLAN.md §5."

## 4. Joint validation and the merge decision

- Before merging two perf branches, create a temporary joint branch that merges
  both: `joint/<a>-<b>`.
- Measure all four points with the §5 protocol: **base → branch A → branch B →
  joint**.
- The merge is approved when **either** condition holds:
  - **(a)** the joint shows the pre-recorded numeric improvement, **or**
  - **(b)** there is no measured gain on the current processor, **but** the
    change makes the code more readable, **or** consumes less memory, **or**
    should theoretically improve performance on other processors, **and** it
    does **not** decrease performance on the current processor. In that case
    the implementation is considered a success.
- If the joint is worse than the base, or brings no gain and no redeeming
  quality (readability / memory / other-hardware potential), **deep-think
  whether the implementation is needed at all** — do not merge on autopilot.
- After a branch is merged, **delete it**: `git branch -d <branch>` locally,
  and `git push origin --delete <branch>` if it was pushed. Clean up the
  temporary `joint/*` branches the same way.

## 5. Benchmark measurement protocol

Follow this exactly so numbers are comparable between base, branches, and the
joint. Adapt the commented values to the machine; the sequence is the point.

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
#    Half-of-max is deterministic and machine-independent: it works on any
#    CPU regardless of its supported frequency range (unlike a hardcoded
#    value, which can sit below or above what a given machine supports)
MAX_FREQ=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq)  # after turbo is off
HALF_FREQ=$((MAX_FREQ / 2))
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies
echo "$HALF_FREQ" | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq
echo "$HALF_FREQ" | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_min_freq

cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq	# to verify

# 6. Isolate cores 1-4 for the measurement (adjust the set)
sudo cset shield -c 1-4 -k on

# 7. Measure: 10 repetitions, isolated cores, pinned frequency
cset shield --exec -- perf stat -r 10 <cmd>
```

Notes:
- `scaling_available_frequencies` may be empty (intel_pstate with HWP):
  frequencies are then continuous, and any value in `[min, max]` is honored.
- If the frequency write is rejected (`Invalid argument`), pick the entry from
  `scaling_available_frequencies` closest to half of the max and use it.
- Compare runs only when both were taken under the same protocol on the same
  machine state. Record raw numbers (e.g. the `perf stat` time) in the todo
  file next to the expected improvement.
