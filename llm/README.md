# llm/ — graphify and review tools for LLM sessions

Thin wrappers around the [graphify](https://github.com/safishamsi/graphify)
CLI for this repo's merged knowledge graph (`graphify-out/graph.json`).
Every script runs from the repo root.

## Scripts

| Script | What it does |
|---|---|
| `graphify-query "<question>"` | BFS traversal for a question. Flags: `--dfs` (trace a chain), `--budget N` (cap tokens), `--graph <path>` |
| `graphify-explain "<node>"` | Plain-language explanation of one node and its connections |
| `graphify-path "A" "B"` | Shortest path between two concepts, hop by hop |
| `graphify-god-nodes [--top N]` | Most-connected nodes (architectural hubs) |
| `graphify-vocab` | Writes `graphify-out/.vocab.txt` — token vocabulary of all node labels. Use it to expand a query against the graph's own vocabulary (the matcher is literal: no stemming, no synonyms) |
| `graphify-reflect` | Aggregates prior Q&A feedback into `graphify-out/reflections/LESSONS.md` and prints it. Run at the start of graph work |
| `graphify-update` | **Code-only refresh, no LLM, no API key.** Re-extracts changed `src/` (AST, cached), regenerates the agent semantic layer (`gen-semantic.py`), re-merges, re-clusters, re-applies paper↔code bridges, regenerates report + HTML |
| `gen-semantic.py` | CMake semantic layer: targets + `target_link_libraries` from `src/**/CMakeLists.txt` → `graphify-out/src-semantic.json` (deterministic `conceptually_related_to` edges, `_origin: agent-semantic`). Runs inside `graphify-update`; idempotent |
| `gen-llm-graph.py` | Regenerates `graphify-out/llm-graph.json` — `document`/`concept`/`rationale` nodes for all `llm/*.md` docs + `references` edges. Agent extraction; no API key |
| `graphify-refresh` | Full refresh: semantic re-extraction of `docs/papers/` via Gemini (needs `GEMINI_API_KEY`/`GOOGLE_API_KEY`; unchanged files cache-skipped), then `graphify-update`. Without a key falls back to code-only with a warning |
| `review_prepare.py` | Targeted by `make review`; resolves the upstream diff, writes `bin/session_brief.md`, and generates granular phase todos |

## Review Preparation

Read `llm/review.md` for the review preparation contract. The common command is
`make review`. It supports `commits`, `patches`, and `report` output contracts,
derives the phase count from `HOURS`, and generates Design/Performance
checklists from `RULES.md`/`PERFORMANCE.md` into `bin/` rather than keeping
generated todo templates in this directory.

## Graph layout

Three scopes merged into one: `src-graph.json` (code, AST, deterministic),
`docs-graph.json` (papers, semantic), `llm-graph.json` (docs, agent
extraction), plus `src-semantic.json` (CMake layer, injected by
`graphify-update`) → `graph.json` (~12.9k nodes / ~25.4k edges + 780
INFERRED paper↔code bridges).

Outputs in `graphify-out/`: `graph.html` (interactive, auto-aggregated
community view), `GRAPH_REPORT.md` (god nodes, communities, suggested
questions), `graph.json`, `.graphify_labels.json` (community names, reused
across updates), `.graphify_analysis.json` (communities/cohesion/god
nodes), `cost.json` (cumulative extraction token tracker).

## Notes

- Shrink guard: `graphify-update` refuses to overwrite if the rebuild has
  fewer nodes than the existing graph; if the reduction is legitimate
  (deleted files), rebuild per `llm/GRAPHIFY.md`.
- Paper semantic re-extraction costs tokens; code-only `graphify-update`
  — including the agent-built CMake/llm semantic layer — is free.
- Community ids may shift on re-cluster; labels are matched by key, new
  communities fall back to `Community N`.
