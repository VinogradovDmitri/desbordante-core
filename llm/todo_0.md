# todo_0.md — PREWORK environment check (phase 0, before todo_1)

> **Template, not a live todo.** At the start of **any** session, copy
> this file into `bin/`:
>
>     mkdir -p bin && cp llm/todo_0.md bin/todo_0.md
>
> and check every applicable box. If any check **fails**, **stop** and
> ask the user to run the relevant `llm/PREWORK.sh` section before
> continuing — do not proceed to `bin/todo_1.md` until this file is
> fully checked and deleted. The `llm/PREWORK.sh` file has the exact
> copy-pasteable commands (sudo / installs the LLM cannot run).
>
> - Mark a checkbox `in_progress` before checking it, `completed`
>   immediately after (S3); keep `bin/todo_0.md` and the in-session
>   display in sync.
> - Skip sections that don't apply to this session (mark N/A), but
>   **never skip a section that applies and leave it unchecked**.
> - **Delete `bin/todo_0.md`** once all applicable checks pass — phase 0
>   is done, proceed to `bin/todo_1.md` (S1).

## A. Fresh-machine setup (always — PREWORK §A)

- [ ] `.venv/` exists: `.venv/bin/python3 --version` succeeds
- [ ] `clang-format` in venv: `.venv/bin/clang-format --version`
      (v22.x — 21.x ≠ 22.x output)
- [ ] `cmake-format` in venv: `.venv/bin/cmake-format --version`
- [ ] Build tools on PATH: `which g++ cmake ninja` (or `ninja-build`)
- [ ] `uv` on PATH: `which uv` (needed for graphify CLI too)

## B. Profiling tools (Performance / perf tasks only — PREWORK §B)

- [ ] `perf` available: `which perf` or `perf --version`
- [ ] `valgrind` available: `which valgrind`
- [ ] (benchmark only) `taskset` available: `which taskset`
- [ ] (allocation profiling) `heaptrack` available: `which heaptrack`

## C. Benchmark measurement protocol (benchmark sessions only — PREWORK §C)

- [ ] CPU governor = performance:
      `cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor` →
      "performance"
- [ ] ASLR disabled: `cat /proc/sys/kernel/randomize_va_space` → "0"
- [ ] unattended-upgrades stopped:
      `systemctl is-active unattended-upgrades` → "inactive"
- [ ] turbo disabled: `cat /sys/devices/system/cpu/intel_pstate/no_turbo`
      → "1" (if intel_pstate)

## D. Gate

- [ ] All applicable checks above pass — if any fail, **stop** and ask
      the user to run the relevant `llm/PREWORK.sh` section; do not start
      `bin/todo_1.md` until this file is fully checked.
- [ ] **Delete `bin/todo_0.md`** (phase 0 done — all checks passed).
