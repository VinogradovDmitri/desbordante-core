# DEVELOPMENT.md — Building, Testing, and Formatting Desbordante

Practical, verified commands for working on the core library: building, running
tests for a single algorithm, exercising the Python bindings and example
snapshots, formatting, and running benchmarks.

Everything below was verified on a Linux box with GCC 16 (`/opt/gcc-16/bin/c++`)
and Python 3.14.4 (`.venv`). Adjust compiler/Python paths for other machines;
the general flow is identical.

> This guide is algorithm-agnostic. Substitute `<algo>` (snake_case, e.g.
> `ga_rfd`), `<AlgoClass>` (Python class name, e.g. `GaRfd`), and
> `<algo-regex>` (CTest filter, e.g. `GARfd|GaRfd|Rfd`) with the algorithm you
> are working on. Everything is a template — never assume an option name,
> metric, or path exists for your algorithm without checking its
> implementation and examples.

## 0. Fresh-machine setup (first task on a new user PC)

Bootstrap the environment before anything else. On this machine everything
below is already done and verified (Python 3.14.4 in `.venv`, uv 0.12.1,
clang-format 22.1.8, cmake-format 0.6.13). Adjust versions for other machines;
the flow is identical.

### 0.1 System packages (Ubuntu)

```sh
sudo apt update && sudo apt upgrade
sudo apt install g++ cmake ninja-build libboost-all-dev python3 python3-venv libicu-dev
export CXX=g++
```

### 0.2 Install uv

```sh
curl -LsSf https://astral.sh/uv/install.sh | sh   # or: sudo snap install uv, pipx install uv
uv --version
```

`uv` is used for fast venv/pip operations and for the graphify CLI — install
it now (the knowledge-graph tooling is dead without it, see `llm/GRAPHIFY.md`):

```sh
uv tool install graphifyy
graphify --version
```

### 0.3 Create the venv

```sh
python3 -m venv .venv
# alternative: uv venv .venv
```

### 0.4 Install all requirements

```sh
uv pip install -r llm/requirements.txt
```

`llm/requirements.txt` is generated from `pip freeze` — the exact dev-venv
reproduction (formatting tools, pytest/snapshottest, example deps, transitives;
Python 3.14.4). Regenerate it when venv deps change:
`.venv/bin/pip freeze` and update the file. `desbordante` is intentionally
absent — the bindings come from `./build.sh -p` (section 3). The CI-pinned
alternative is `examples/test_examples/test_examples_requirements.txt`
(Python 3.10-era pins; resolves on 3.14 as well); the local venv may carry
newer versions of the same packages — both are fine.

### 0.5 Formatting tools

Skip this step if you ran §0.4: `llm/requirements.txt` already pins
`clang-format==22.1.8` and `cmake-format==0.6.13`. Otherwise install them
explicitly:

```sh
uv pip install "clang-format==22.1.8"   # same version CI uses
uv pip install cmake-format             # provides cmake-format, cmake-lint, cmake-annotate
uv pip install uv                       # uv inside the venv (optional)
```

### 0.6 Verify

```sh
.venv/bin/python3 --version
.venv/bin/clang-format --version
.venv/bin/cmake-format --version
uv --version
graphify --version
```

## 1. Building

The project is built with the bundled `./build.sh` wrapper (a thin layer over
CMake + Ninja). The build directory is `build/`.

### Build parallelism

The number of build threads is **CNT_CPU_CORE / 2**, where CNT_CPU_CORE is the
count of **physical cores** on the machine: every core counted once,
hyper-threads excluded. On hybrid CPUs the E-cores count as cores too — only
hyper-threads are excluded. On this machine that is 8 physical cores → `-j4`.

**First task on any user PC: adjust the build to that machine.** Never reuse
`-j` values from another machine. Compute CNT_CPU_CORE on the machine itself
(the unique `SOCKET x CORE` pairs):

```bash
lscpu -e                # the SOCKET x CORE pairs column = physical cores
lscpu -p | awk -F, '!/^#/ {print $2" "$3}' | sort -u | wc -l
```

then build with `-j<CNT_CPU_CORE / 2>` (integer division, at least 1). The
test, performance, and "just check" builds all follow this rule; all commands
below use `-j4` — this machine's value (8 physical cores / 2).

### Plain Debug build with tests

```bash
./build.sh -j4 -f -b -p -d
```

| Flag   | Meaning                                              |
| ------ | ---------------------------------------------------- |
| `-j4`  | 4 parallel jobs (CNT_CPU_CORE / 2 = 8 physical cores / 2) |
| `-f`   | don't re-fetch datasets (they are already in-tree)   |
| `-b`   | build benchmarks (`build/target/Desbordante.benchmark`) |
| `-p`   | build the Python bindings (pybind11 module)          |
| `-d`   | Debug build type                                     |
| `-s[=S]` / `-C[OPT]` | see below                                     |

### Debug build with ASan + UBSan together

The `-s` flag supports only one sanitizer at a time (`ADDRESS` or `UB`), but you
can combine them by forwarding CMake flags. This combination matches what CI
runs — **one build runs both sanitizers at once; never do two separate
sanitizer builds** (each build consumes a lot of time):

```bash
./build.sh -j4 -f -b -p -d --sanitizer=UB \
  -C'-DCMAKE_CXX_FLAGS_DEBUG=-fsanitize=address' \
  -C'-DCMAKE_EXE_LINKER_FLAGS_DEBUG=-fsanitize=address'
```

Notes:

- `--sanitizer=UB` adds `-fsanitize=undefined ...` to the Debug flags;
  the `-C` overrides add Address Sanitizer on top. Both sanitizers then run in
  one binary.
- Debug builds print a pybind11 `FutureWarning` about an old-style
  `__setstate__` on stderr when importing the module — harmless, visible only
  in debug builds.
- The compiler is taken from `CMAKE_CXX_COMPILER` in the build dir. On this
  machine: `build/CMakeCache.txt` has `CMAKE_CXX_COMPILER=/opt/gcc-16/bin/c++`.
- The built Python module:
  `build/src/python_bindings/desbordante.cpython-<ver>-x86_64-linux-gnu.so`
  (built against the venv Python, e.g. `cpython-314`).

### Two build configurations: final tests vs. performance measurement

Do not benchmark a debug/sanitizer build, and do not declare a task done from
a plain Release build. Each configuration has a single command:

**Final test build — all available test flags (Debug + ASan + UBSan + bindings
+ benchmarks, matches CI):**

```bash
./build.sh -j4 -f -b -p -d --sanitizer=UB \
  -C'-DCMAKE_CXX_FLAGS_DEBUG=-fsanitize=address' \
  -C'-DCMAKE_EXE_LINKER_FLAGS_DEBUG=-fsanitize=address'
```

Then verify with the full suite: `ctest --test-dir build`, the example
snapshots, and the cross-process determinism probe (sections 2-3). This build
is used for correctness only — its numbers say nothing about performance.

**Performance measurement build — only `-p -b -j4 -f`:**

```bash
./build.sh -p -b -j4 -f
```

| Flag   | Meaning                                                        |
| ------ | -------------------------------------------------------------- |
| `-p`   | Python bindings — measure through the public Python API        |
| `-b`   | benchmarks (`build/target/Desbordante.benchmark`)              |
| `-j4`  | 4 parallel jobs (CNT_CPU_CORE / 2 = 8 physical cores / 2)      |
| `-f`   | don't re-fetch datasets (already in-tree)                      |

Every flag that would distort the measurement is deliberately absent:

| Omitted flag | Effect of omission |
| ------------ | ------------------ |
| `-d` (no Debug) | `CMAKE_BUILD_TYPE=Release` — `-O3 -DNDEBUG` |
| `-s` / `-C` (no sanitizers) | no ASan/UBSan instrumentation in the binary |
| `-g` (no GDB debug info) | no debug symbols |

Rationale: debug symbols and sanitizers consume a lot of memory and decrease
performance, so timings from such a build are not representative of the code
that would actually ship. Measure only with the Release build and only under
the protocol in `llm/PLAN.md` §5 (pinned CPU, isolated cores, `perf stat -r 10`);
see `llm/PERFORMANCE.md` §1 and §12 for the full measurement checklist.

Note: without `-n`, tests are still compiled (just not run) — add `-n` if build
time matters; it does not affect runtime measurement.

## 2. Running tests for one algorithm

Tests are CTest targets registered by `desbordante_add_test`. Use `ctest` with a
regular expression filter over target names:

```bash
ctest --test-dir build -R "<algo-regex>"
```

- The `-R` filter matches any part of the test target name
  (e.g. `<Algo>Determinism.*`, `<Algo>Threads.*`, `<Algo>DatasetTest.*`).
  Derive it from `src/tests/unit/test_<algo>.cpp` and the algorithm's own
  name.
- If unsure what matches, list the tests first:
  `ctest --test-dir build -N | grep -i <algo>`.
- The full suite is `ctest --test-dir build` (slow — prefer `-R`).

### Cross-process determinism probe

Unit tests cover in-process reproducibility but may miss cross-process
reproducibility at the default thread count (this is how a real data race in a
threaded bitset build was caught). For any randomized algorithm, probe by
spawning fresh processes with the same seed and asserting identical output:

```bash
export PYTHONPATH=$PWD/build/src/python_bindings
export LD_PRELOAD="$(/opt/gcc-16/bin/c++ -print-file-name=libasan.so) $(/opt/gcc-16/bin/c++ -print-file-name=libubsan.so)"
export ASAN_OPTIONS=detect_leaks=0
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

> Option names (`seed`, `threads`, …), the module path
> (`desbordante.<pattern>.algorithms`), and result accessors are
> algorithm-specific — verify them against the algorithm's bindings
> (`src/python_bindings/<pattern>/`) and examples before adapting.

## 3. Python bindings and example snapshots

### Environment

Use the venv (`pytest`, `snapshottest`, `cmake-format`, … live there) and point
`PYTHONPATH` at the freshly built module so the examples run against *your*
build, not any pip-installed `desbordante`:

```bash
export PATH=$PWD/.venv/bin:$PATH
export PYTHONPATH=$PWD/build/src/python_bindings
export LD_PRELOAD="$(/opt/gcc-16/bin/c++ -print-file-name=libasan.so) $(/opt/gcc-16/bin/c++ -print-file-name=libubsan.so)"
export ASAN_OPTIONS=detect_leaks=0
export MPLBACKEND=Agg   # skip plt.show() in examples
```

- The harness (`examples/test_examples/test_examples_pytest.py`) runs each
  example as `python3 examples/<script>` with the above environment and
  compares stdout against snapshots in
  `examples/test_examples/snapshots/snap_test_examples_pytest.py`.
- Examples need `pandas`, `tabulate`, `networkx`, `termcolor`, and (for some)
  `jellyfish`, `ordered_set`, `colorama`, `matplotlib`. Install what's missing
  into the venv: `.venv/bin/pip install jellyfish ordered_set colorama matplotlib`.
- CI pins exact versions in `examples/test_examples/test_examples_requirements.txt`
  (Python 3.10); the local venv may carry newer versions. If an example's
  output depends on a pinned library's formatting, regenerate with the CI
  versions to be safe.

### Run / update snapshots

```bash
# Run the example tests for one algorithm only
python3 -m pytest examples/test_examples/test_examples_pytest.py -k <algo>

# Regenerate snapshots for those examples
python3 -m pytest examples/test_examples/test_examples_pytest.py --snapshot-update -k <algo>
```

> **Warning:** `--snapshot-update -k <filter>` rewrites the whole snapshot file
> from the *executed* tests only — it **drops every deselected snapshot**.
> To update a subset, back up the file first, or run the full
> `--snapshot-update` (slow — runs every example) and keep only the wanted
> entries.

Snapshot file notes:

- Format: `snapshots['<full test name>'] = '''<exact stdout>'''`, one blank
  line between entries, values kept verbatim (ANSI escapes are written as
  literal `\x1b` in the source; they evaluate to real ESC bytes when the file
  is imported).
- The `-k <algo>` filter matches test *node* ids of the form
  `test_example[basic/mining_<algo>.py-None-mining_<algo>_output]`. List them
  with `python3 -m pytest examples/test_examples/test_examples_pytest.py --collect-only -q | grep <algo>`.

### Binding smoke test

`src/python_bindings/test_bindings.py` exercises options/types on all
algorithms (`_set_option`, `_get_option_type`, `_get_needed_options` — the
internal API; the public API is keyword arguments of `load_data()`/`execute()`
plus `set_option` on released pip builds).

## 4. Format and static checks

CI runs these on every PR (`.github/workflows/check-codestyle.yml`):

| Check            | CI tool                                            | Local equivalent |
| ---------------- | -------------------------------------------------- | ---------------- |
| clang-format     | `clang-format-22` (apt, LLVM 22) — diff-based, `continue-on-error` | `.venv/bin/clang-format -i <files>` (v22 via pip) |
| clang-tidy       | `ZedThree/clang-tidy-review` (`.clang-tidy`, blocking) | not installed locally; CI-only |
| cmake-format     | apt `cmake-format`, checks **all** CMake files     | `.venv/bin/cmake-format --check <file>` |
| typos            | `crate-ci/typos` (blocking)                        | not installed locally; CI-only |

Install the exact CI clang-format version locally (pip wheels exist for 22.x):

```bash
.venv/bin/pip install "clang-format==22.1.8"
.venv/bin/clang-format -i <changed .cpp/.h files>
```

`clang-format-diff.py` (also in `.venv/bin`) can be used for diff-only checks
like CI does. Prefer formatting the whole files you touch, with the same
version CI uses (21.x vs 22.x produce different output!).

## 5. Benchmarks

```bash
./build.sh -j4 -f -b -p -d ...   # -b builds the benchmark target
build/target/Desbordante.benchmark --help
```

Benchmark code lives in `src/tests/benchmark/<algo>_benchmark.h` and is
registered through `src/tests/benchmark/CMakeLists.txt`. Benchmark sources are
developed without committing them.

For performance work (branches, joint validation, merge decisions), use the
official measurement protocol in `llm/PLAN.md` §5.

## 6. Before declaring a task done — run these in order

For any code change, verify before stopping. Each check must be reported with
its command and result; checks that cannot be run locally (clang-tidy, typos)
must be reported as "not run locally — CI-only", never as passed.

**Conditional loop:** if a check fails, fix and re-run starting from the
first failed check — never skip a check. Record every command + result in
`bin/session_<YYYY-MM-DD>.md` (see `llm/CLAUDE.md` §0).

```bash
# 1. Rebuild with the same flags used before the change
./build.sh -j4 -f -b -p -d --sanitizer=UB \
  -C'-DCMAKE_CXX_FLAGS_DEBUG=-fsanitize=address' \
  -C'-DCMAKE_EXE_LINKER_FLAGS_DEBUG=-fsanitize=address'

# 2. Targeted tests for the touched algorithm(s)
ctest --test-dir build -R "<algo-regex>"

# 3. Example scripts + snapshots (with the env block from section 3)
python3 -m pytest examples/test_examples/test_examples_pytest.py -k <algo>

# 4. Formatting (CI uses clang-format 22 exactly)
.venv/bin/clang-format -i <changed .cpp/.h files>

# 5. CMake formatting (CI checks ALL CMake files, not only changed ones)
.venv/bin/cmake-format --check <changed CMakeLists.txt/*.cmake>
```

Notes:

- If the examples' output changed intentionally, regenerate snapshots with
  `--snapshot-update`, then re-run without it (plain rerun must pass) — and
  remember the `-k` warning from section 3.
- CI-only, not runnable locally: clang-tidy (`ZedThree/clang-tidy-review`) and
  typos (`crate-ci/typos`).
- After formatting, rebuild and re-run step 2 if anything changed.
