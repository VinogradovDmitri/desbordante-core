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
interpreter it uses is printed by the `bin/` wrappers), and this repo's merged
graph already exists in `graphify-out/` — the quick start above is only for a
fresh machine or a full rebuild from scratch (Steps 1-9 of the `/graphify`
skill).

## Workflow: before, during, after

**Before** starting a non-trivial task, make sure the graph exists and is
current — `graphify-out/` is not tracked (excluded via `.git/info/exclude`,
per-clone), so the graph is **not part of the
repo** and a fresh clone has no graph at all:

- `graphify-out/graph.json` missing → **create it first**: run the full
  `/graphify` skill pipeline (Steps 1-9), or `bin/graphify-refresh` when a
  `graphify-out/cache/` already exists.
- Graph exists but code changed since it was built → **update it first** with
  `bin/graphify-update` (free, AST-only).

On a fresh clone, also re-add the local excludes (they live in `.git/info/exclude`,
per-clone, not in the repo): `bin/` and `graphify-out/`.

Then map dependencies:

- `bin/graphify-explain "<component>"` — what the component is connected to
- `bin/graphify-path "A" "B"` — dependency chain between two components
  (relations show calls vs. shared structures vs. paper-documented links)
- `bin/graphify-query "<question>"` — e.g. "which modules will a change to X
  affect?", "which papers document algorithm Y?"
- `bin/graphify-reflect` — read prior Q&A lessons first

**During** work, answer structural questions with query/path/explain instead of
greps. Dirty `graphify-out/` files after an update are expected.

**After** all work is done:

1. Code changed → `bin/graphify-update` (free, AST-only; re-extracts changed
   files, re-merges, re-clusters, regenerates report + HTML).
2. Docs/papers changed → `bin/graphify-refresh` (adds semantic re-extraction;
   needs `GEMINI_API_KEY`).
3. Report the refresh in the final summary with the resulting node/edge counts.

Skip the graph only if the task is about stale/incorrect graph output or the
user says not to use it.

## Where things live

- `graphify-out/graph.json` — the merged graph (nodes + edges; ~12.5k nodes)
- `graphify-out/GRAPH_REPORT.md` — god nodes, surprising connections, suggested
  questions, community list
- `graphify-out/graph.html` — interactive visualization (community view)
- `bin/` — wrapper scripts (see `bin/README.md`)
- `graphify-out/reflections/LESSONS.md` — accumulated Q&A feedback

## How to answer a question with the graph

1. **Start of session**: run `bin/graphify-reflect` and read the printed lessons
   (preferred sources, dead ends, corrections).
2. **Expand the query against the graph's vocabulary.** The matcher is literal
   (case-folded substring; no stemming, no synonyms). Run `bin/graphify-vocab`
   once, read `graphify-out/.vocab.txt`, and pick up to 12 tokens from that file
   that match the question's intent (cross-language: Russian "статистика" →
   look for `statistic`). Never invent tokens. If nothing matches, say so.
3. **Traverse**: `bin/graphify-query "<expanded tokens>"` (BFS default; `--dfs`
   to trace a chain; `--budget N` to cap output). For a specific node:
   `bin/graphify-explain "<node>"`. For a relation between two concepts:
   `bin/graphify-path "A" "B"`.
4. **Answer only from the graph.** Quote `source_location` when citing a fact
   (e.g. `core/algorithms/fd/hyfd/...` L42). If the graph lacks the information,
   say so. Never invent edges.
5. **Save the result** (feedback loop):
   `graphify save-result --question "<original question>" --answer "<answer>" --type query --nodes <cited node labels>`
   and add `--outcome useful|dead_end|corrected` (with `--correction` when wrong).

## Honesty rules

- Paper↔code bridge edges (relation `semantically_similar_to`, `INFERRED`,
  confidence 0.85, `_origin: paper-code-bridge`) are model-reasoned: they match a
  paper's title stem to the code module that implements it. Treat them as
  suggestions, not facts.
- `AMBIGUOUS` edges (confidence 0.1-0.3) are flagged for review — do not cite
  them as fact.
- If the graph has no relevant vocabulary for a question, say so instead of
  substituting near-synonyms.

## Keeping the graph fresh

- After code changes: `bin/graphify-update` — free (AST only), re-extracts
  changed files, re-merges, re-clusters, regenerates report + HTML.
- After doc/paper changes (or to redo semantic extraction): `bin/graphify-refresh`
  — needs `GEMINI_API_KEY` (`pip install 'graphifyy[gemini]'`); without a key it
  falls back to code-only with a warning.
- A full rebuild from scratch follows the graphify skill pipeline (Steps 1-9 in
  the `/graphify` skill).
- The shrink guard: an update that would shrink the graph refuses to overwrite.
  If files were intentionally deleted, pass the `--force` equivalent via a full
  rebuild.
