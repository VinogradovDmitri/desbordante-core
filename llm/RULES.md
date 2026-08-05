# RULES.md — Algorithm Compliance Checklist (Desbordante)

Step-by-step checklist. Review every box, in order. See `AGENT.md` for scope,
workflow, and output format.

## 1. Supported compilers & build

- [ ] Builds on Linux GCC 10+
- [ ] Builds on Linux LLVM Clang 16+
- [ ] Builds on macOS Apple Clang 16+
- [ ] Builds on macOS GNU GCC 10+
- [ ] Builds on macOS LLVM Clang 16+

## 2. Non-standard GCC / libstdc++ extensions

- [ ] No libstdc++ language extensions (e.g. no variable-length arrays)
- [ ] No libstdc++ STL extensions (no `_`/`__`-prefixed library names)
- [ ] SGI `std::bitset` extensions replaced by `src/core/model/types/bitset.h`
- [ ] Feature-test macros used instead of `__GNUG__` / `__clang__`

## 3. Inline namespaces

- [ ] No use of `std::__detail`, `std::__cxx11`, `std::_V2`, or other
      implementation-detail inline namespaces (even on GCC)

## 4. `template` disambiguator

- [ ] Dependent type names use `typename`
- [ ] Dependent template names use `.template` / `->template` / `::template`

## 5. Linker / ABI

- [ ] `CXXFLAGS` and `LDFLAGS` point to one compatible stdlib version (per README)
- [ ] Full paths used when multiple versions of a compiler are installed
- [ ] boost is built with the same compiler as the code

## 6. Cross-configuration behavior

- [ ] No reliance on implementation-defined behavior (e.g. order of equal elements
      after `std::sort`; use `std::stable_sort` if order matters)
- [ ] No undefined behavior missed by sanitizers (e.g. narrowing float→int)

## 7. Sanitizers

- [ ] Passes UB sanitizer on GCC and Clang
- [ ] Passes Address sanitizer on GCC and Clang
- [ ] Any suppression (`no_sanitize` / ignore list) is a confirmed false positive,
      minimal in scope, with the equivalent GCC check left enabled

## 8. CMake (target-based build)

- [ ] Correct target types: internal libs `OBJECT`, header-only umbrella
      `INTERFACE` (with `FILE_SET HEADERS`), user-facing algorithm `LIBRARY`;
      no manual `add_executable`
- [ ] One `CMakeLists.txt` per directory containing a source file (or a single
      target for the whole algorithm)
- [ ] Each `CMakeLists.txt` is pulled in via `add_subdirectory` from its parent only
      (tree mirrors the directory tree)
- [ ] Target created with `desbordante_add_lib(NAME <TYPE>)`; `${NAME}` used
      afterward, never the literal name
- [ ] Main target named `<pattern>.<algo>`
- [ ] `target_sources` lists only `.cpp` files (except header-only targets)
- [ ] `PUBLIC` deps are those used in headers; `PRIVATE` deps are those used only
      in `.cpp`
- [ ] Desbordante deps use `${DESBORDANTE_PREFIX}::…`; links to `::algos`, not
      `::create_algo` (cyclic dependency)
- [ ] Directory added to `SUBDIRS` in `src/core/algorithms/CMakeLists.txt`
- [ ] Main target added to the `create_algo` dependency list
- [ ] No missing and no superfluous dependencies (a "file not found" build error
      means a dep is missing or wrongly PRIVATE — fix the linkage)
- [ ] All CMake files pass `cmake-format` with the project `.cmake-format.yaml`

## 9. Bindings — mechanism

- [ ] Added via `desbordante_add_bind(<name> SRCS … LIBS …)` in
      `src/python_bindings/CMakeLists.txt`
- [ ] `LIBS` includes every library whose types are bound (plus `Boost::headers`
      if needed)

## 10. Bindings — required functionality

- [ ] Instances of the primitive are obtainable in Python
- [ ] Instances are obtainable one-by-one via iteration, and their total count is
      available
- [ ] Each instance is printable directly (`to_string`)
- [ ] Each instance is processable by parts: get left part, get right part,
      enumerate right-part attributes, get threshold X, get threshold Y, etc.
- [ ] Instances are comparable and hashable / placeable in a set (to support
      intersect / subtract of sets)
- [ ] The primitive object is constructible, and miner and validator objects
      coincide (a mined object can be inserted straight into the validator)
- [ ] Instances are serializable and deserializable, preserving maximum
      information (e.g. table schema)
- [ ] Stubs updated (Desbordante/desbordante-stubs)
- [ ] Console/CLI output considered (Desbordante/desbordante-cli)
- [ ] Ability to pass a user-defined metric (Python code) into the C++ core (where
      applicable)
- [ ] Conversion strategy considered for objects with vs. without extra metrics
      (e.g. CFD confidence/support)

## 11. Exceptions / explanations

- [ ] Uses clusters (preferred) or pair-sets; a new form is discussed if neither
      fits
- [ ] Returns a minimal suspicious set, not the whole dataset
- [ ] Cluster-based: clusters enumerable and countable
- [ ] Cluster-based: per cluster — set of constituent rows
- [ ] Cluster-based: per cluster — count of those rows
- [ ] Cluster-based: per cluster — most frequent value
- [ ] Pair-based: pairs enumerable and countable
- [ ] Computation of exceptions during validation is toggle-able (considered)

## 12. Usage example

- [ ] States the primitive with its source paper (name, authors, year,
      conference/journal)
- [ ] Mentions other examples if any (mining/validation counterpart;
      exact/approximate pattern and algorithm) — breadth
- [ ] Defines the primitive and explains it on a real example
- [ ] Describes algorithm parameters: what they do and allowed values
- [ ] Mentions multiple algorithms if applicable
- [ ] Prints and explains the dataset (≤ 15 rows, ≤ 6 columns; smaller is better;
      larger only if unavoidable, e.g. sampling)
- [ ] Dataset stored in `examples/datasets`; path in code is from repo root
      (`examples/datasets/...`)
- [ ] Searches/verifies and shows working techniques (iterate instances, get right
      part, print)
- [ ] Shows behavior change when key parameters or data change (find error → fix →
      recheck → better result)
- [ ] References the next example for this primitive if one exists
- [ ] Example added to CI and `snapshot-*` updated
- [ ] Snapshot regenerated via the example harness and verified against live
      output (run `--snapshot-update`, then a plain rerun must pass)
- [ ] All supported built-in metrics described (if the primitive uses them)
- [ ] User-defined-metric example provided (if supported)
- [ ] If randomized: stated explicitly with reason, seed fixed, and seed verified
      to reproduce results on another machine (e.g. Colab)
- [ ] Error-finding shown for left and right parts (two examples, if applicable)
- [ ] Mentions experimentation (parameter tuning, typo hunting)
- [ ] Language polished (LLM), text split into blank-line-separated paragraphs,
      key points optionally highlighted

## 13. Standalone data structures

- [ ] Algorithm-produced structures (e.g. PLI) are usable standalone, with minimal
      coupling to algorithm objects and without hurting performance

## 14. C++ style

- [ ] Follows Google C++ style with project exceptions: 100-char lines; exceptions
      allowed; all numeric built-in types allowed; 4-space indentation; `.cpp`
      extension; `#pragma once` allowed; files/dirs `snake_case`; access-modifier
      order private → protected → public
- [ ] No mutable static or thread-local storage-duration variables

## 15. Logging

- [ ] Logs are for debugging only, never end-user information
- [ ] `{fmt}` syntax used, never string concatenation
- [ ] Correct levels: TRACE (per-iteration low-level), DEBUG (execution steps/key
      vars), INFO (high-level progress), WARN (non-critical recoverable), ERROR
      (critical, often before `throw`), CRITICAL (crash/data-corruption)
- [ ] Custom types logged via templated `operator<<` (preferred) or `fmt::formatter`
- [ ] Expensive log preparation in hot loops guarded by
      `#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG` and kept at DEBUG+

## 16. Option & exception types (visibility)

- [ ] Non-standard option types marked with `DESBORDANTE_EXPORT`
- [ ] Exception types marked with `DESBORDANTE_EXPORT`
- [ ] New option type registered in `py_to_any.cpp` (mandatory)
- [ ] New option type registered in `opt_to_py.cpp` (when possible)

## 17. Pattern objects (what you need to be able to do)

- [ ] Returned `get_*()` objects have independent lifetime (ideally plain strings
      and numbers); new patterns do not use `RelationalSchema`, `Column`, `Vertical`
- [ ] Serialization is simple (tuple of standard/previously-bound types)
- [ ] `__eq__` / `__hash__` implemented on bindings; `__eq__` via
      `py::self == py::self` (`operator==`)
- [ ] `__str__` / `__repr__` provided; a constructor matching `__repr__` is
      available
- [ ] Miner output convertible to validator input directly, or via a single method
      call (Fd ↔ FdInput)
- [ ] Object modeled close to the paper's definition
- [ ] Access is convenient (e.g. column names, not indices, for validators)
- [ ] Unicode support preserved
- [ ] Class roles separated (representation / internal / result-storage / input);
      representation is user-facing and convertible to input; validator accepts
      representation (or an easy-to-construct input class); storage stores compactly
      and yields representation objects (e.g. `transform_view`, `py::make_iterator`,
      `reference_internal`); algorithm holds a `shared_ptr` to storage, overwritten
      on rerun

## 18. Development philosophy

- [ ] Implements only the efficiently and unambiguously implementable "core"
- [ ] NP-hard / approximate work is pushed to Python, not the core (e.g. DC minimal
      deletion set)
- [ ] No over-engineering (e.g. no explicit column selection for FD); small
      high-value low-cost additions are acceptable (e.g. header-row handling)
- [ ] No standalone theoretical features (consistency / derivability / triviality /
      minimality) beyond what a mining/validation algorithm itself needs
- [ ] Data structures are duplicated rather than shared; no mutation of a shared
      structure for one algorithm's benefit
- [ ] `Execute` does not corrupt input; repeated `Execute` calls stay correct
- [ ] Input data read only in `LoadData`; `Execute` and `ResetState` do not re-read
      input
- [ ] No extraneous code (unused/rarely-used code, leftover benchmarking/
      performance-measurement code removed)
- [ ] Every claim verified by actually running the checks in
      `llm/DEVELOPMENT.md` (build, targeted `ctest`, example `pytest`,
      formatting) — nothing is assumed; CI-only checks explicitly reported as
      not run locally
- [ ] Clarifying questions asked before implementation when the task is
      ambiguous or the change would touch code outside the request's scope
      (see `llm/CLAUDE.md`)

## 19. CI awareness — Python wheels

- [ ] Aware that the wheel matrix runs on push to `main`, weekly (Mon 03:00 UTC),
      and on release; skipped on PRs unless the `python-packaging-risk` label is
      added (removing it cancels the run)
