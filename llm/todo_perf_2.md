# todo_perf_2.md — Performance review checklist: PERFORMANCE.md §2 Algorithmic optimizations (the 100× territory)

> **Template, not a live todo.** At the start of a **Performance-mode**
> review, copy this file into `bin/`:
>
>     cp llm/todo_perf_2.md bin/todo_perf_2.md
>
> then walk it top to bottom as your own todo file for the §2 phase
> (`llm/AGENT.md` "Review workflow"; `llm/CLAUDE.md` §10 S2;
> `llm/PLAN.md` §2).
>
> - Mark a checkbox `in_progress` before reviewing it, `completed`
>   immediately after (S3); keep the `bin/todo_perf_2.md` file and the
>   in-session todo display in sync (same message).
> - A box is `completed` only after its verification actually ran —
>   command executed / log inspected / result recorded in the session log
>   (S5).
> - Record each box's verdict (**Pass / Fail / N/A / Needs-info**) with
>   `file:line` and *how verified* (build / profiling / inspection only)
>   in the review report (path per `llm/AGENT.md` "Review defaults"), not here.
> - Walk top to bottom, in order; every box gets a verdict (S4).
> - **Delete `bin/todo_perf_2.md` once every checkbox below is
>   `completed`** — a fully checked file is removed after the phase's
>   verification pass (S1; `llm/CLAUDE.md` §0 item 1; `llm/PLAN.md` §2). A
>   leftover completed file is a tracking failure.

Source: `llm/PERFORMANCE.md` §2 "Algorithmic optimizations (the 100×
territory)".
Acceptance criterion: every §2 box has a verdict in the report, this
file is fully checked, and it is deleted.

## Workflow (commit-based — per template)

- [ ] Baseline measurement (quick — not a long benchmark run; record
      numbers in `bin/measurements_<YYYY-MM-DD>.md`)
- [ ] Implementation — walk the §2 checklist below, applying the
      optimizations
- [ ] Measurement after implementation — compare against baseline
      (`llm/PLAN.md` §5 protocol)
- [ ] Create a new commit if there is a measured performance increase;
      if no increase, revert so the working tree returns to baseline
- [ ] Delete this `bin/todo_perf_2.md` file (S1; all checkboxes
      completed)

## §2 — Algorithmic optimizations (the 100× territory)

- [ ] §2 — **Do less work first**: reduce the search space before
      validating
- [ ] §2 — Prune with every mathematically valid rule (minimality,
      non-triviality, monotonicity/anti-monotonicity,
      known-valid/invalid subsets and supersets); never generate
      supersets of valid minimal keys or known determinants
- [ ] §2 — Re-read the paper + follow-up variants; verify every pruning
      rule it defines is actually implemented
- [ ] §2 — Detect constant and unique columns once, early; prune
      candidates they make trivial; drop exact-duplicate columns when
      appropriate
- [ ] §2 — **Sample-first validation** (HyFD): reject against a small
      random sample before full partitions; verify survivors exactly —
      sampling as a filter, never the final answer
- [ ] §2 — **Stripped partitions**: drop singleton equivalence classes
      everywhere
- [ ] §2 — **Refine, don't rebuild**: derive π(X∪A) from a parent
      partition
- [ ] §2 — **Cache expensive reusable results** with a reuse-likelihood
      policy (intersections, error metrics, cardinalities, closures,
      sorted orders, dictionaries, prefix hashes)
- [ ] §2 — Early termination: stop at the first counterexample; stop
      error accumulation once the threshold is exceeded (lower bound
      first)
- [ ] §2 — Order candidates by cost: ascending cardinality/selectivity,
      likely-to-fail first; high distinct counts early for uniqueness,
      low for dependency elimination
- [ ] §2 — Canonical candidate representation (sorted IDs/bitsets);
      deduplicate candidate lists; deterministic order for reproducible
      runs
- [ ] §2 — Level-wise processing: level k+1 only from valid level-k
      candidates; prefix grouping; release finished-level structures
- [ ] §2 — Subset-check representation: bitsets for moderate column
      counts, sorted vectors for sparse/large; cache repeated checks
- [ ] §2 — Hitting-set/dualization for sparse search spaces;
      incremental computation everywhere (extend, don't restart)
- [ ] §2 — Algorithm by table shape: sampler-based (HyFD) for wide
      tables, level-wise (TANE/Pyro) for narrow
- [ ] §2 — Check complexity constants — O(n log n) with huge constants
      loses on real data
