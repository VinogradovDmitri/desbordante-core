# llm/ — graphify tools for LLM sessions

Thin wrappers around the [graphify](https://github.com/safishamsi/graphify) CLI for
this repo's merged knowledge graph (`graphify-out/graph.json`). Every script runs
from the repo root, so they work from any working directory.

## Scripts

| Script | What it does |
|---|---|
| `graphify-query "<question>"` | BFS traversal over the graph for a question. Flags: `--dfs` (trace a chain), `--budget N` (cap tokens), `--graph <path>` |
| `graphify-explain "<node>"` | Plain-language explanation of one node and its connections |
| `graphify-path "A" "B"` | Shortest path between two concepts, hop by hop |
| `graphify-god-nodes [--top N]` | Most-connected nodes (architectural hubs) |
| `graphify-vocab` | Writes `graphify-out/.vocab.txt` — the token vocabulary of all node labels. Use it to expand a query against the graph's own vocabulary before running `graphify-query` (the matcher is literal: no stemming, no synonyms) |
| `graphify-reflect` | Aggregates prior Q&A feedback into `graphify-out/reflections/LESSONS.md` and prints it (preferred sources, dead ends, corrections). Run at the start of graph work |
| `graphify-update` | **Code-only refresh, no LLM, no API key.** Re-extracts changed code in `src/` (AST, cached), regenerates the agent-built semantic layer (`gen-semantic.py` → `src-semantic.json`), re-merges with the papers/llm graphs, re-clusters, re-applies paper↔code bridges, regenerates `GRAPH_REPORT.md` + `graph.html` |
| `gen-semantic.py` | Agent-built semantic layer: parses all `CMakeLists.txt` under `src/` (targets + `target_link_libraries`) into `graphify-out/src-semantic.json` — deterministic `conceptually_related_to` edges, `_origin: agent-semantic`. Runs inside `graphify-update`; idempotent |
| `gen-llm-graph.py` | Regenerates `graphify-out/llm-graph.json` — `document`/`concept`/`rationale` nodes for all 7 `llm/*.md` docs plus `references` edges. Agent extraction; no API key |
| `graphify-refresh` | Full refresh: semantic re-extraction of `docs/papers/` via Gemini (needs `GEMINI_API_KEY` or `GOOGLE_API_KEY`; unchanged files are cache-skipped), then runs `graphify-update`. Without a key it falls back to code-only with a warning. The `llm/` + CMake semantic layers no longer need it |

## Graph layout

The graph is built from three scopes, merged into one:

```
src-graph.json    code: 1,022 C++ files in src/ (AST extraction, deterministic)
docs-graph.json   papers: 110 PDFs in docs/papers/ (semantic extraction)
llm-graph.json    docs: 7 guidance files in llm/ (agent extraction, no key)
src-semantic.json CMake layer: targets + conceptual edges (agent extraction,
                  deterministic, injected into graph.json by graphify-update)
graph.json        merged: ~12.9k nodes / ~25.4k edges + 780 INFERRED
                  paper↔code bridge edges (_origin: paper-code-bridge)
```

Outputs in `graphify-out/`:

- `graph.html` — interactive visualization (auto-aggregated community view; the
  graph exceeds 5,000 nodes)
- `GRAPH_REPORT.md` — audit report: god nodes, surprising connections, suggested
  questions, communities with cohesion scores
- `graph.json` — node-link graph; `graphify query/path/explain` read it directly
- `.graphify_labels.json` — community names, reused across updates
- `.graphify_analysis.json` — communities, cohesion, god nodes, surprises
- `cost.json` — cumulative extraction token tracker

## Notes

- `graphify-update` refuses to overwrite if the rebuild has fewer nodes than the
  existing graph (shrink guard). If the reduction is legitimate (files deleted),
  rebuild per `llm/GRAPHIFY.md`.
- Paper semantic re-extraction (`graphify-refresh`) costs tokens; code-only
  `graphify-update` — including the agent-built CMake/llm semantic layer — is free.
- Community ids may shift on re-cluster; labels are matched by key, and new
  communities fall back to `Community N`.
