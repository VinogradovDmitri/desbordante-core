# GRAPHIFY.md — knowledge graph guide

Persistent knowledge graph built from `src/` (C++ code, AST), `docs/papers/`
(semantic extraction) and `llm/` (guidance docs), merged into one. Use it to
answer codebase questions without re-reading files. Mandatory workflow:
**consult the graph before and during work; refresh it after work**
(`llm/CLAUDE.md` §7).

## Get started (fresh machine only)

```bash
uv tool install graphifyy   # or: pipx install graphifyy
graphify install            # register the skill with your AI assistant
```

Then `/graphify .` in the assistant → `graphify-out/` with `graph.html`
(interactive), `GRAPH_REPORT.md` (highlights), `graph.json` (the full
graph). On this machine `graphifyy` is already installed and the graph
exists — the quick start is only for a fresh machine or a full rebuild from
scratch (skill Steps 1-9).

## Workflow: before, during, after

**If the graph needs creating/updating, ask the user first whether they can
run it.** If yes — hand them the exact command and wait for their call-back.
If not — use the existing `graphify-out/graph.json` as-is and state its
vintage when citing it.

**Before** a non-trivial task: `graphify-out/` is untracked (per-clone
`.git/info/exclude`), so a fresh clone has no graph:
- Missing → create: full `/graphify` skill pipeline, or
  `llm/graphify-refresh` when `graphify-out/cache/` exists.
- Stale (code newer than the graph — check with
  `find src docs llm -newer graphify-out/graph.json -print -quit`) →
  `llm/graphify-update` first. Acting on a stale graph is worse than on
  none.
- On a fresh clone, re-add the local excludes: `bin/` and `graphify-out/`.

Then map dependencies: `llm/graphify-explain "<component>"`,
`llm/graphify-path "A" "B"`, `llm/graphify-query "<question>"`,
`llm/graphify-reflect` (prior lessons first).

**During** work: answer structural questions with query/path/explain
instead of greps. Dirty `graphify-out/` after updates is expected.

**After** work: code changed → `llm/graphify-update` (free, AST-only);
docs/papers changed → `llm/graphify-refresh` (adds paper semantic
re-extraction; needs `GEMINI_API_KEY`, else code-only fallback with a
warning). Report the refresh in the final summary with node/edge counts.
Skip the graph only for stale-graph tasks or on the user's word.

## Where things live

- `graphify-out/graph.json` — the merged graph (~12.5k nodes)
- `graphify-out/GRAPH_REPORT.md`, `graphify-out/graph.html` — report + viz
- `graphify-out/reflections/LESSONS.md` — accumulated Q&A feedback
- `llm/` wrapper scripts — see `llm/README.md`

## How to answer a question with the graph

1. **Start of session**: `llm/graphify-reflect`, read the printed lessons
   (preferred sources, dead ends, corrections).
2. **Expand the query against the graph's vocabulary.** The matcher is
   literal (case-folded substring; no stemming, no synonyms). Run
   `llm/graphify-vocab` once, pick up to 12 tokens from
   `graphify-out/.vocab.txt` matching the intent (cross-language: Russian
   "статистика" → `statistic`). Never invent tokens; say so if nothing
   matches.
3. **Traverse**: `llm/graphify-query "<tokens>"` (BFS default; `--dfs` to
   trace a chain; `--budget N` to cap output); `llm/graphify-explain
   "<node>"`; `llm/graphify-path "A" "B"`. Reading output: `-->` outgoing,
   `<--` incoming, `[EXTRACTED]` edges are deterministic fact,
   `[INFERRED]` are model suggestions; high `Degree` = architectural hub
   (`llm/graphify-god-nodes`).
4. **Answer only from the graph.** Quote `source_location` when citing a
   fact; say so when the graph lacks the information; never invent edges.
5. **Save the result** (feedback loop):
   `graphify save-result --question "…" --answer "…" --type query --nodes
   <cited node labels>` with `--outcome useful|dead_end|corrected`
   (`--correction` when wrong).

## The semantic layer (no API key needed)

AST extraction cannot see CMake structure (`target_link_libraries`,
aggregators, bindings module). `llm/graphify-update` rebuilds it:
- `llm/gen-semantic.py` — parses every `CMakeLists.txt` under `src/` into
  `graphify-out/src-semantic.json`: target nodes + deterministic
  `conceptually_related_to` edges (`EXTRACTED`, `_origin: agent-semantic`).
  Idempotent — repeated runs don't accumulate.
- `llm/gen-llm-graph.py` — `graphify-out/llm-graph.json`: one `document`
  node per `llm/*.md` + `concept`/`rationale` sub-nodes and `references`
  edges (`EXTRACTED` where docs literally cross-reference, `INFERRED` for
  reasoned similarity).

The paper layer still comes from keyed extraction (`llm/graphify-refresh`).
Keep ids `repo::src_*` (code) and `llm_*` (docs) for stable merges.

## Honesty rules

- Paper↔code bridges (`semantically_similar_to`, `INFERRED`,
  `_origin: paper-code-bridge`) are model-reasoned suggestions, not facts.
- `agent-semantic` edges are deterministic (verbatim
  `target_link_libraries`) — citable as fact, like AST edges.
- `INFERRED` edges are reasoned suggestions; prefer `EXTRACTED` as evidence.
- If the graph has no relevant vocabulary for a question, say so instead of
  substituting near-synonyms.
- If the generators change, re-run `llm/graphify-update` (idempotent).

## Keeping the graph fresh

- Code changes → `llm/graphify-update` (free).
- Doc/paper changes or redo of paper extraction → `llm/graphify-refresh`
  (needs `GEMINI_API_KEY`; `pip install 'graphifyy[gemini]'`).
- Full rebuild → the `/graphify` skill pipeline (Steps 1-9).
- Shrink guard: an update that would shrink the graph refuses to overwrite;
  if file deletion is intentional, pass the `--force` equivalent via a full
  rebuild.
