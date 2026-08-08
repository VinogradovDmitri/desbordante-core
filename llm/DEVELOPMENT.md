# DEVELOPMENT.md — Building, Testing, Formatting

Verified commands for this repo. Substitute `<algo>` (snake_case, e.g.
`ga_rfd`), `<AlgoClass>` (Python class, e.g. `GaRfd`), `<algo-regex>`
(ctest filter, e.g. `GARfd|GaRfd|Rfd`) per algorithm. Never assume an
option name, metric, or path exists without checking the implementation and
examples.

## 0. Fresh-machine setup (first task on a new PC)

Already done and verified on this machine (Python 3.14.4 in `.venv`, uv
0.12.1, clang-format 22.1.8, cmake-format 0.6.13). For a fresh machine:

```sh
# 1. System packages (Ubuntu)
sudo apt update && sudo apt upgrade
sudo apt install g++ cmake ninja-build libboost-all-dev python3 python3-venv libicu-dev
export CXX=g++

# 2. uv (needed for the graphify CLI too — llm/GRAPHIFY.md)
curl -LsSf https://astral.sh/uv/install.sh | sh   # or: sudo snap install uv / pipx install uv
uv tool install graphifyy && graphify --version

# 3. venv + requirements
python3 -m venv .venv
uv pip install -r llm/requirements.txt

# 4. Verify
.venv/bin/python3 --version && .venv/bin/clang-format --version && .venv/bin/cmake-format --version
```

- `llm/requirements.txt` is `pip freeze` output — regenerate with
  `.venv/bin/pip freeze` when deps change. `desbordante` is intentionally
  absent (bindings come from `./build.sh -p`).
- CI-pinned alternative for examples:
  `examples/test_examples/test_examples_requirements.txt`.

## 1. Building

Wrapper `./build.sh` over CMake+Ninja; build dir `build/`.

### Build parallelism
`-j<CNT_CPU_CORE / 2>` — physical cores only (each counted once,
hyper-threads excluded; E-cores count). **First task on any user PC:
recompute there**; never reuse `-j` from another machine. Count:
`lscpu -e` (SOCKET x CORE pairs). This machine: 8 physical cores → `-j4`.
Applies to test, perf, and just-check builds alike.

### Flags
| Flag | Meaning |
|---|---|
| `-j4` | jobs (this machine: 8 cores / 2) |
| `-f` | don't re-fetch datasets (already in-tree) |
| `-b` | build benchmarks (`build/target/Desbordante.benchmark`) |
| `-p` | build Python bindings (pybind11 module) |
| `-d` | Debug build type |

### Test build (Debug, bindings, benchmarks)
Local build for running tests and examples — no sanitizers (covered by CI):

```bash
./build.sh -j4 -f -b -p -d
```

Notes: Debug builds print a harmless pybind11 `FutureWarning` about
`__setstate__` on stderr. Compiler from `build/CMakeCache.txt` (this
machine: `/opt/gcc-16/bin/c++`). Built module:
`build/src/python_bindings/desbordante.cpython-<ver>-x86_64-linux-gnu.so`.

### Two build configurations
Never benchmark a debug build; never declare a task done from a
plain Release build.

- **Final test build** (Debug + bindings + benchmarks): the command above,
  then **targeted tests only** — `ctest --test-dir build -R "<algo-regex>"`
  (§2 — never the full suite), example snapshots, and the determinism
  probe (§2-3). Correctness only — its numbers say nothing about
  performance. Sanitizers (ASan/UBSan) are run by CI only — no local
  sanitizer builds.
- **Performance measurement build** — `./build.sh -p -b -j4 -f`: Release
  (`-O3 -DNDEBUG`), deliberately no `-d`, no `-g` — debug symbols consume
  memory and distort timings. Measure only under the `llm/PLAN.md` §5
  protocol. Without `-n` tests are still compiled (not run) — add `-n` if
  build time matters.

### Environmental pitfalls (verified on this machine — check before building)

- **Never build in `/tmp`** — it is a small tmpfs here (7.5 G). The Debug
  build exceeds 4.5 G of object files and fills it; symptoms are
  misleading: `No space left on device` while writing `.o` files, then
  `as: BFD (GNU Binutils for Ubuntu) 2.46 assertion fail ../../bfd/elf.c:3571`
  and failing `ar`/link steps that look like a toolchain bug. Check
  `df -h /tmp /home` first; keep the worktree and `build/` on the main
  disk (`/home` has 250+ G free here).
- **Worktree builds need a datasets symlink, correctly made** — `datasets/`
  is a *tracked* directory (it contains `datasets/CMakeLists.txt`), so a
  plain `ln -s <repo>/datasets <worktree>/datasets` creates a **nested**
  symlink (`<worktree>/datasets/datasets`), and configure fails with
  "Cannot access `<worktree>/datasets/datasets.zip`". Fix: remove the
  tracked dir first, then symlink:
  `rm -rf <worktree>/datasets && ln -s <repo>/datasets <worktree>/datasets`
  (same for any tracked dir you replace with a symlink).
- **Worktrees live in `bin/`** — `git worktree add bin/<name> <branch>`
  works (git only blocks an already-used branch); all agent-created dirs
  stay under `bin/` per `llm/CLAUDE.md` §0. Moving a worktree later bakes
  old paths into `build/CMakeCache.txt` → full rebuild; place it correctly
  the first time. `-j6` is safe on this machine (14 Gi RAM, ~1 Gi per
  compiler job); watch `free -m` and drop back to `-j4` if swap pressure
  grows.
- **The branch must be available to the user when work is done** — a
  branch checked out in a worktree is locked (`git switch <branch>` →
  `fatal: '<branch>' is already used by worktree at '...'`). After the
  last commit, free it: `git worktree remove --force bin/<name>` (commit
  is safe in git; only the local build cache is lost). Do this before
  reporting done, or the user cannot check the branch out.

## 2. Running tests for one algorithm

```bash
ctest --test-dir build -R "<algo-regex>"
```

- `-R` matches test target names (e.g. `<Algo>Determinism.*`,
  `<Algo>Threads.*`); derive from `src/tests/unit/test_<algo>.cpp`. List
  first if unsure: `ctest --test-dir build -N | grep -i <algo>`.
- **Never run the full suite locally** (`ctest --test-dir build` without
  `-R`) — it runs tests for every algorithm and is CI-only; always scope
  with `-R` to the algorithm under work.

### Cross-process determinism probe
Unit tests can miss cross-process reproducibility at the default thread
count (this is how a real data race in a threaded bitset build was caught).
Probe: spawn fresh processes with the same seed, assert identical output:

```bash
export PYTHONPATH=$PWD/build/src/python_bindings
python3 - <<'EOF'
import subprocess, collections
script = '''
from desbordante.<pattern>.algorithms import <AlgoClass>
algo = <AlgoClass>()
algo.load_data(table=('<path/to/dataset>', ',', True))
algo.execute(seed=42, threads=<default_threads>)   # adapt option names!
print(len(algo.get_<results>()))
'''
counts = collections.Counter()
for _ in range(30):
    out = subprocess.run(['python3', '-c', script], capture_output=True, text=True)
    assert out.returncode == 0, out.stderr[-300:]
    counts[out.stdout.strip()] += 1
print(dict(counts))  # all runs must produce the same value
EOF
```

Option names (`seed`, `threads`, …), the module path
(`desbordante.<pattern>.algorithms`), and accessors are algorithm-specific —
check the bindings (`src/python_bindings/<pattern>/`) and examples first.

## 3. Python bindings and example snapshots

```bash
export PATH=$PWD/.venv/bin:$PATH
export PYTHONPATH=$PWD/build/src/python_bindings
export MPLBACKEND=Agg   # skip plt.show() in examples
```

- The harness runs each example as `python3 examples/<script>` with the
  above environment and compares stdout against snapshots in
  `examples/test_examples/snapshots/snap_test_examples_pytest.py`.
- Examples need `pandas`, `tabulate`, `networkx`, `termcolor`, and (some)
  `jellyfish`, `ordered_set`, `colorama`, `matplotlib` — install into the
  venv what's missing. CI pins exact versions in
  `examples/test_examples/test_examples_requirements.txt`.

```bash
python3 -m pytest examples/test_examples/test_examples_pytest.py -k <algo>
python3 -m pytest examples/test_examples/test_examples_pytest.py --snapshot-update -k <algo>
```

> **`--snapshot-update -k <filter>` rewrites the whole snapshot file from
> the executed tests only — it drops every deselected snapshot.** Back up
> first, or run the full update and keep only the wanted entries.

Snapshot format: `snapshots['<full test name>'] = '''<exact stdout>'''`,
one blank line between entries, values verbatim (ANSI escapes as literal
`\x1b`). Node ids look like `test_example[basic/mining_<algo>.py-None-…]` —
list with `python3 -m pytest … --collect-only -q | grep <algo>`.

Binding smoke test: `src/python_bindings/test_bindings.py` exercises
options/types on all algorithms (internal API).

## 4. Format and static checks

| Check | CI tool | Local equivalent |
|---|---|---|
| clang-format | `clang-format-22` (apt, diff-based) | `.venv/bin/clang-format -i <files>` (v22 via pip) |
| clang-tidy | `ZedThree/clang-tidy-review` (blocking) | CI-only, not installed locally |
| cmake-format | apt, checks **all** CMake files | `.venv/bin/cmake-format --check <file>` |
| typos | `crate-ci/typos` (blocking) | CI-only, not installed locally |

```bash
.venv/bin/pip install "clang-format==22.1.8"   # same version CI uses (21.x ≠ 22.x output)
.venv/bin/clang-format -i <changed .cpp/.h files>
```

`clang-format-diff.py` (in `.venv/bin`) does diff-only checks like CI.

## 5. Benchmarks

```bash
./build.sh -j4 -f -b -p -d ...   # -b builds the benchmark target
build/target/Desbordante.benchmark --help
```

Benchmark code lives in `src/tests/benchmark/<algo>_benchmark.h`
(developed without committing). For performance work (branches, joint
validation, merge decisions), use the official measurement protocol in
`llm/PLAN.md` §5.

## 6. Before declaring a task done — in order

Each check reported with its command and result; CI-only checks reported as
"not run locally — CI-only", never passed. **Conditional loop:** on failure
fix and re-run starting from the first failed check. Record everything in
`bin/session_<YYYY-MM-DD>.md`.

```bash
# 1. Rebuild with the same flags used before the change
./build.sh -j4 -f -b -p -d

# 2. Targeted tests for the touched algorithm(s)
ctest --test-dir build -R "<algo-regex>"

# 3. Example scripts + snapshots (with the env block from section 3)
python3 -m pytest examples/test_examples/test_examples_pytest.py -k <algo>

# 4. Formatting (CI uses clang-format 22 exactly)
.venv/bin/clang-format -i <changed .cpp/.h files>

# 5. CMake formatting (CI checks ALL CMake files, not only changed ones)
.venv/bin/cmake-format --check <changed CMakeLists.txt/*.cmake>
```

Notes: intentional output change → `--snapshot-update`, then a plain rerun
must pass (remember the `-k` warning from §3). CI-only: clang-tidy, typos.
After formatting, rebuild and re-run step 2 if anything changed.
