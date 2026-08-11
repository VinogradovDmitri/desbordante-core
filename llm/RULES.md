# RULES.md — Algorithm Compliance Checklist (Desbordante)

Checklist of checkable requirements. Review every box, in order. See
`AGENT.md` for scope, workflow, and output format.

## 1. Supported compilers & build
- [ ] Builds on Linux GCC 10+ and Linux LLVM Clang 16+
- [ ] Builds on macOS Apple Clang 16+, macOS GNU GCC 10+, macOS LLVM Clang 16+

## 2. Non-standard GCC / libstdc++ extensions
- [ ] No libstdc++ language extensions (e.g. variable-length arrays)
- [ ] No libstdc++ STL extensions (`_`/`__`-prefixed library names)
- [ ] SGI `std::bitset` extensions replaced by `src/core/model/types/bitset.h`
- [ ] Feature-test macros used instead of `__GNUG__` / `__clang__`

## 3. Inline namespaces
- [ ] No implementation-detail inline namespaces (`std::__detail`,
      `std::__cxx11`, `std::_V2`, …)

## 4. `template` disambiguator
- [ ] Dependent type names use `typename`
- [ ] Dependent template names use `.template` / `->template` / `::template`

## 5. Linker / ABI
- [ ] `CXXFLAGS`/`LDFLAGS` point to one compatible stdlib version (per README)
- [ ] Full paths used when multiple compiler versions are installed
- [ ] boost built with the same compiler as the code

## 6. Cross-configuration behavior
- [ ] No reliance on implementation-defined behavior (e.g. equal-element
      order after `std::sort`; `std::stable_sort` if order matters)
- [ ] No UB missed by CI sanitizers (e.g. narrowing float→int) — CI runs
      ASan/UBSan

## 7. Sanitizers
- [ ] No local sanitizer builds required — ASan/UBSan are run by CI
      (GitHub Actions); a local build must not be a sanitizer build
- [ ] Any suppression (`no_sanitize`/ignore list) is a confirmed false
      positive, minimal in scope, equivalent GCC check left enabled

## 8. CMake (target-based build)
- [ ] Correct target types: internal `OBJECT`, header-only umbrella
      `INTERFACE` (`FILE_SET HEADERS`), user-facing `LIBRARY`; no manual
      `add_executable`
- [ ] One `CMakeLists.txt` per directory with sources (or one target for the
      whole algorithm); each pulled in via `add_subdirectory` from its parent
      only
- [ ] Target via `desbordante_add_lib(NAME <TYPE>)`; `${NAME}` used
      afterward, never the literal name; main target named `<pattern>.<algo>`
- [ ] `target_sources` lists only `.cpp` files (except header-only targets)
- [ ] `PUBLIC` = deps used in headers; `PRIVATE` = deps used only in `.cpp`
- [ ] Desbordante deps use `${DESBORDANTE_PREFIX}::…`; link `::algos`, not
      `::create_algo` (cyclic dependency)
- [ ] Directory added to `SUBDIRS` (`src/core/algorithms/CMakeLists.txt`);
      main target in the `create_algo` dependency list
- [ ] No missing/superfluous deps ("file not found" = missing or wrongly
      PRIVATE — fix the linkage)
- [ ] All CMake files pass `cmake-format` with `.cmake-format.yaml`

## 9. Bindings — mechanism
- [ ] Added via `desbordante_add_bind(<name> SRCS … LIBS …)` in
      `src/python_bindings/CMakeLists.txt`
- [ ] `LIBS` includes every library whose types are bound (plus
      `Boost::headers` if needed)

## 10. Bindings — required functionality
- [ ] Instances obtainable in Python; iterable one-by-one with total count
- [ ] Each instance printable directly (`to_string`)
- [ ] Processable by parts (left part, right part, enumerate right
      attributes, thresholds X/Y, …)
- [ ] Comparable and hashable / placeable in a set (set intersect/subtract)
- [ ] Constructible; miner and validator objects coincide (mined object
      inserted straight into the validator)
- [ ] Serializable/deserializable preserving maximum information (e.g. schema)
- [ ] Stubs updated (Desbordante/desbordante-stubs); console/CLI output
      considered (Desbordante/desbordante-cli)
- [ ] User-defined metric (Python) passable into the C++ core where applicable
- [ ] Conversion strategy considered for objects with vs. without extra
      metrics (e.g. CFD confidence/support)

## 11. Exceptions / explanations
- [ ] Uses clusters (preferred) or pair-sets; a new form discussed if
      neither fits
- [ ] Returns a minimal suspicious set, not the whole dataset
- [ ] Cluster-based: clusters enumerable and countable; per cluster — row
      set, row count, most frequent value
- [ ] Pair-based: pairs enumerable and countable
- [ ] Exception computation during validation toggle-able (considered)

## 12. Usage example
- [ ] States the primitive with its source paper (name, authors, year, venue)
- [ ] Mentions other examples (mining/validation counterpart; exact/
      approximate) — breadth
- [ ] Defines the primitive on a real example; describes parameters and
      allowed values; mentions multiple algorithms if applicable
- [ ] Dataset printed ≤ 15 rows / ≤ 6 columns (smaller is better; larger
      only if unavoidable); stored in `examples/datasets`, path from repo
      root
- [ ] Searches/verifies with working techniques (iterate instances, get
      right part, print)
- [ ] Shows behavior change when key parameters/data change (find error →
      fix → recheck → better result)
- [ ] References the next example for this primitive; added to CI,
      `snapshot-*` updated
- [ ] Snapshot regenerated via the harness (`--snapshot-update`) then a
      plain rerun passes
- [ ] All supported built-in metrics described; user-defined-metric example
      if supported
- [ ] If randomized: stated with reason, seed fixed, and seed verified to
      reproduce on another machine (e.g. Colab)
- [ ] Error-finding shown for left and right parts (two examples if
      applicable); mentions experimentation (tuning, typo hunting)
- [ ] Language polished; blank-line-separated paragraphs; key points
      optionally highlighted

## 13. Standalone data structures
- [ ] Algorithm-produced structures (e.g. PLI) usable standalone, minimal
      coupling to algorithm objects, no performance harm

## 14. C++ style
- [ ] Google C++ style with project exceptions: 100-char lines; exceptions
      allowed; all numeric types allowed; 4-space indent; `.cpp` extension;
      `#pragma once`; files/dirs `snake_case`; access order private →
      protected → public
- [ ] No mutable static or thread-local storage-duration variables

## 15. Logging
- [ ] Logs for debugging only, never end-user info; `{fmt}` syntax, never
      string concatenation
- [ ] Levels: TRACE (per-iteration), DEBUG (steps/key vars), INFO
      (progress), WARN (non-critical), ERROR (critical, often before
      `throw`), CRITICAL (crash/data-corruption)
- [ ] Custom types via templated `operator<<` (preferred) or `fmt::formatter`
- [ ] Expensive log preparation in hot loops guarded by
      `#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG`, kept at DEBUG+

## 16. Option & exception types (visibility)
- [ ] Mark all non-C++-standard types used as template parameter for 
      config::Option<T> with DESBORDANTE_EXPORT 
      (defined in src/core/util/export.h) to avoid macOS linking issues. 
      Typical symptoms are errors like "Incorrect type for option ..." or 
      "Cannot get type for option ..."
- [ ] New option type registered in `py_to_any.cpp` (mandatory), in
      `opt_to_py.cpp` (when possible)

## 17. Pattern objects (what you need to be able to do)
- [ ] `get_*()` objects have independent lifetime (ideally plain
      strings/numbers); new patterns do not use `RelationalSchema`,
      `Column`, `Vertical`
- [ ] Serialization simple (tuple of standard/previously-bound types)
- [ ] `__eq__`/`__hash__` implemented; `__eq__` via `py::self == py::self`
      (`operator==`)
- [ ] `__str__`/`__repr__` provided; a constructor matching `__repr__`
      available
- [ ] Miner output convertible to validator input directly, or via a single
      method call (Fd ↔ FdInput)
- [ ] Modeled close to the paper's definition; access convenient (e.g.
      column names, not indices); Unicode preserved
- [ ] Class roles separated (representation / internal / result-storage /
      input): representation user-facing, convertible to input; validator
      accepts representation or an easy input class; storage compact,
      yields representation (`transform_view`, `py::make_iterator`,
      `reference_internal`); algorithm holds a `shared_ptr` to storage,
      overwritten on rerun

## 18. Development philosophy
- [ ] Implements only the efficiently and unambiguously implementable
      "core"; NP-hard/approximate work pushed to Python (e.g. DC minimal
      deletion set)
- [ ] No over-engineering (e.g. no explicit column selection for FD); small
      high-value low-cost additions acceptable (e.g. header-row handling)
- [ ] No standalone theoretical features (consistency/derivability/
      triviality/minimality) beyond what a mining/validation algorithm needs
- [ ] Data structures duplicated rather than shared; no mutation of a shared
      structure for one algorithm's benefit
- [ ] `Execute` does not corrupt input; repeated `Execute` calls stay correct
- [ ] Input read only in `LoadData`; `Execute`/`ResetState` do not re-read
      input
- [ ] No extraneous code (unused/rarely-used code, leftover benchmarking/
      performance-measurement code removed)
- [ ] Every claim verified by actually running the checks in
      `llm/DEVELOPMENT.md`; CI-only checks explicitly reported as not run
      locally
- [ ] Clarifying questions asked before implementation when the task is
      ambiguous or touches code outside the request's scope
      (see `llm/CLAUDE.md`)
- [ ] All agent-created files live in `bin/` — todo/session/measurement
      logs, review reports, and every temp/scratch file (profiling data,
      dumps, one-off scripts, artifacts); never the opencode dir, repo
      root, `llm/`, or `graphify-out/` (see `llm/CLAUDE.md` §0)
- [ ] Todo/session discipline per `llm/CLAUDE.md` §0/§10 (S-rules):
      per-phase `bin/todo_<num>.md` files created first, real-time
      statuses, deleted when fully done — no exceptions for task size

## 19. CI awareness — Python wheels
- [ ] Aware that the wheel matrix runs on push to `main`, weekly (Mon 03:00
      UTC), and on release; skipped on PRs unless the
      `python-packaging-risk` label is added (removing it cancels the run)

## 20. Superior rules — todo & checklist discipline (S1–S6)

Mandatory in every session. Full text and authority: `llm/CLAUDE.md` §10 —
S1 todo files per phase, S2 transcribe the governing checklist (for Design
that is this file's §1–§19), S3 live statuses, S4 in-order walk, S5
completed = verified, S6 startup gate. The S-rules are session discipline,
not algorithm requirements: they are not transcribed into Design todos.
