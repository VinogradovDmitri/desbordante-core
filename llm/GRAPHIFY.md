# GRAPHIFY.md — knowledge graph guide for LLM sessions

This repo has a persistent knowledge graph built from `src/` (C++ code, AST
extraction), `docs/papers/` (110 course papers, semantic extraction) and `llm/`
(guidance docs), merged into one graph. Use it to answer codebase questions
without re-reading files. Mandatory workflow: **consult the graph before and
during work; refresh it after work** (see `llm/CLAUDE.md` §7).

## Get started

```bash
uv tool install graphifyy   # install the CLI (or: pipx install graphifyy)
graphify install            # register the skill with your AI assistant
```

Then, in your AI assistant:

```
/graphify .
```

That's it. You get three files:

```
graphify-out/
├── graph.html       open in any browser — click nodes, filter, search
├── GRAPH_REPORT.md  the highlights: key concepts, surprising connections, suggested questions
└── graph.json       the full graph — query it anytime without re-reading your files
```

On this machine `graphifyy` is already installed as a `uv tool` (the Python
interpreter it uses is printed by the `llm/` wrappers), and this repo's merged
graph already exists in `graphify-out/` — the quick start above is only for a
fresh machine or a full rebuild from scratch (Steps 1-9 of the `/graphify`
skill).

## Workflow: before, during, after

**If the graph needs to be created or updated, ask the user first whether
they can run it.** If yes — hand them the exact command, let them run it
manually, and wait for their call-back. If not — use the existing
`graphify-out/graph.json` as-is and state its vintage (last update) when
citing it.

**Before** starting a non-trivial task, make sure the graph exists and is
current — `graphify-out/` is not tracked (excluded via `.git/info/exclude`,
per-clone), so the graph is **not part of the
repo** and a fresh clone has no graph at all:

- `graphify-out/graph.json` missing → **create it first**: run the full
  `/graphify` skill pipeline (Steps 1-9), or `llm/graphify-refresh` when a
  `graphify-out/cache/` already exists.
- Graph exists but code changed since it was built → **update it first** with
  `llm/graphify-update` (free, AST-only).
- Staleness check before consulting: if anything under `src/`, `docs/`, or
  `llm/` is newer than the graph
  (`find src docs llm -newer graphify-out/graph.json -print -quit` prints a
  file), run `llm/graphify-update` **before** answering from it — free,
  code-only, no API key needed. Acting on a stale graph is worse than acting
  on none.

On a fresh clone, also re-add the local excludes (they live in `.git/info/exclude`,
per-clone, not in the repo): `bin/` and `graphify-out/`.

Then map dependencies:

- `llm/graphify-explain "<component>"` — what the component is connected to
- `llm/graphify-path "A" "B"` — dependency chain between two components
  (relations show calls vs. shared structures vs. paper-documented links)
- `llm/graphify-query "<question>"` — e.g. "which modules will a change to X
  affect?", "which papers document algorithm Y?"
- `llm/graphify-reflect` — read prior Q&A lessons first

**During** work, answer structural questions with query/path/explain instead of
greps. Dirty `graphify-out/` files after an update are expected.

**After** all work is done:

1. Code changed → `llm/graphify-update` (free, no API key; re-extracts changed
   code, re-merges, re-clusters, regenerates report + HTML).
2. Docs/papers changed → `llm/graphify-refresh` (adds paper semantic
   re-extraction; needs `GEMINI_API_KEY`). The `llm/` semantic layer needs no
   key — see "The semantic layer" below.
3. Report the refresh in the final summary with the resulting node/edge counts.

Skip the graph only if the task is about stale/incorrect graph output or the
user says not to use it.

## Where things live

- `graphify-out/graph.json` — the merged graph (nodes + edges; ~12.5k nodes)
- `graphify-out/GRAPH_REPORT.md` — god nodes, surprising connections, suggested
  questions, community list
- `graphify-out/graph.html` — interactive visualization (community view)
- `llm/` — wrapper scripts (see `llm/README.md`)
- `graphify-out/reflections/LESSONS.md` — accumulated Q&A feedback

## How to answer a question with the graph

1. **Start of session**: run `llm/graphify-reflect` and read the printed lessons
   (preferred sources, dead ends, corrections).
2. **Expand the query against the graph's vocabulary.** The matcher is literal
   (case-folded substring; no stemming, no synonyms). Run `llm/graphify-vocab`
   once, read `graphify-out/.vocab.txt`, and pick up to 12 tokens from that file
   that match the question's intent (cross-language: Russian "статистика" →
   look for `statistic`). Never invent tokens. If nothing matches, say so.
3. **Traverse**: `llm/graphify-query "<expanded tokens>"` (BFS default; `--dfs`
   to trace a chain; `--budget N` to cap output). For a specific node:
   `llm/graphify-explain "<node>"`. For a relation between two concepts:
   `llm/graphify-path "A" "B"`.

   Example — `graphify explain "GA-RFD"` resolves the name to one node and
   lists its direct connections (live output after a code-only refresh):

   ```
   Node: rfd.ga_rfd — GA-RFD algorithm
     ID:        repo::src_core_algorithms_rfd_ga_rfd_cmakelists_rfd_ga_rfd
     Source:    core/algorithms/rfd/ga_rfd/CMakeLists.txt None
     Type:      code
     Community: None
     Degree:    7

   Connections (7):
     --> config — configuration options library [conceptually_related_to] [EXTRACTED]
     <-- create_algo — algorithm library aggregator (INTERFACE) [conceptually_related_to] [EXTRACTED]
     --> algos — algorithm framework library [conceptually_related_to] [EXTRACTED]
     --> util — shared utility library [conceptually_related_to] [EXTRACTED]
     <-- Desbordante.benchmark — benchmark executable [conceptually_related_to] [EXTRACTED]
     <-- desbordante [conceptually_related_to] [EXTRACTED]
     --> rfd — relaxed functional dependencies library [conceptually_related_to] [EXTRACTED]
   ```

   Reading it: `-->` is an outgoing edge (this node relates to the target),
   `<--` is incoming (the target references this node — here `create_algo`
   aggregates the algorithm, `desbordante` binds it, the benchmark links it).
   `[relation]` says how the two are linked; `[EXTRACTED]` edges come from
   deterministic extraction and are citable as fact, while `INFERRED` edges
   are model-reasoned suggestions (Honesty rules below). `Degree` is the total
   connection count — high-degree nodes are architectural hubs
   (`llm/graphify-god-nodes`).
4. **Answer only from the graph.** Quote `source_location` when citing a fact
   (e.g. `core/algorithms/fd/hyfd/...` L42). If the graph lacks the information,
   say so. Never invent edges.
5. **Save the result** (feedback loop):
   `graphify save-result --question "<original question>" --answer "<answer>" --type query --nodes <cited node labels>`
   and add `--outcome useful|dead_end|corrected` (with `--correction` when wrong).

## The semantic layer (no API key needed)

The AST extraction (code, `imports`/`calls` edges) cannot see CMake-level
structure: `target_link_libraries` links (PUBLIC/PRIVATE), aggregator
libraries (`create_algo`, `algos`), and the Python bindings module
(`desbordante`). Two agent-built generators restore that layer — run
automatically inside `llm/graphify-update` when present:

- `llm/gen-semantic.py` — parses every `CMakeLists.txt` under `src/` for
  targets and link lists and emits `graphify-out/src-semantic.json`: missing
  target nodes + `conceptually_related_to` edges with confidence `EXTRACTED`
  (deterministic, grounded in the file content, `_origin: agent-semantic`).
  It is idempotent: `llm/graphify-update` injects only genuinely new nodes and
  edges, so repeated runs do not accumulate.
- `llm/gen-llm-graph.py` — regenerates `graphify-out/llm-graph.json`: one
  `document` node per `llm/*.md` plus `concept`/`rationale` sub-nodes and
  `references` edges (`EXTRACTED` where the docs literally cross-reference
  each other, `INFERRED` for reasoned similarity, `_origin` absent).

The paper layer (`docs-graph.json`, paper↔code bridges) is untouched by this —
it still comes from the keyed semantic extraction (`llm/graphify-refresh`).
If you extend the generators, keep the ids `repo::src_*` (code) and `llm_*`
(docs) so the merge tags stay stable.

## Honesty rules

- Paper↔code bridge edges (relation `semantically_similar_to`, confidence
  `INFERRED`, `_origin: paper-code-bridge`) are model-reasoned: they match a
  paper's title stem to the code module that implements it. Treat them as
  suggestions, not facts.
- Semantic-layer edges from `llm/gen-semantic.py` (confidence `EXTRACTED`,
  `_origin: agent-semantic`) are deterministic — they reproduce
  `target_link_libraries` content verbatim — so they are citable as fact,
  like AST edges. `llm/gen-llm-graph.py` edges marked `INFERRED` are
  reasoned suggestions.
- The `llm/` and code semantic layer is built by an agent, not a paid
  service; if the generators change, re-run `llm/graphify-update` so the
  graph matches the generators (they are idempotent).
- Edge confidence is categorical (`EXTRACTED` vs `INFERRED`), not numeric —
  prefer `EXTRACTED` edges as evidence and weigh `INFERRED` ones accordingly.
- If the graph has no relevant vocabulary for a question, say so instead of
  substituting near-synonyms.

## Keeping the graph fresh

- After code changes: `llm/graphify-update` — free (AST + agent semantic
  layer), re-extracts changed files, re-merges, re-clusters, regenerates
  report + HTML.
- After doc/paper changes (or to redo paper semantic extraction):
  `llm/graphify-refresh` — needs `GEMINI_API_KEY`
  (`pip install 'graphifyy[gemini]'`); without a key it falls back to
  code-only with a warning.
- A full rebuild from scratch follows the graphify skill pipeline (Steps 1-9 in
  the `/graphify` skill).
- The shrink guard: an update that would shrink the graph refuses to overwrite.
  If files were intentionally deleted, pass the `--force` equivalent via a full
  rebuild.
