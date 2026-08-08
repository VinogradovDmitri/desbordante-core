# GRAPHIFY.md — knowledge graph guide

Persistent knowledge graph built from `src/` (C++ code, AST), `docs/papers/`
(semantic extraction) and `llm/` (guidance docs), merged into one. Use it to
answer codebase questions without re-reading files. Mandatory workflow:
**consult the graph before and during work** (`llm/CLAUDE.md` §7); if the
graph is missing or stale (not updated for a long time), **write a warning
to the user** — never create or refresh it yourself.

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

**The LLM never creates or updates the graph** — those tools are run by
the user (`llm/README.md`). If the graph needs creating/updating, write a
warning to the user instead (per `llm/CLAUDE.md` §7).

**Before** a non-trivial task: `graphify-out/` is untracked (per-clone
`.git/info/exclude`), so a fresh clone has no graph. Check staleness with
`find src docs llm -newer graphify-out/graph.json -print -quit`; if
anything is newer (or the graph is old), **write a warning to the user** —
do not build it yourself. On a fresh clone, re-add the local excludes:
`bin/` and `graphify-out/`.

Then map dependencies: `llm/graphify-explain "<component>"`,
`llm/graphify-path "A" "B"`, `llm/graphify-query "<question>"`,
`llm/graphify-reflect` (prior lessons first).

**During** work: answer structural questions with query/path/explain
instead of greps. Dirty `graphify-out/` after user updates is expected.

**After** work: no graph action — the LLM does not refresh the graph. If
the user wants it refreshed, they run `llm/graphify-update` (code changed)
or `llm/graphify-refresh` (docs/papers changed) themselves.

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
- If the generators change, the user should re-run `llm/graphify-update`
  (idempotent).

## Keeping the graph fresh

These refresh tools are run by the **user**, never by the LLM. If the
graph is stale, the LLM warns the user and leaves the update to them.

- Code changes → `llm/graphify-update` (free).
- Doc/paper changes or redo of paper extraction → `llm/graphify-refresh`
  (needs `GEMINI_API_KEY`; `pip install 'graphifyy[gemini]'`).
- Full rebuild → the `/graphify` skill pipeline (Steps 1-9).
- Shrink guard: an update that would shrink the graph refuses to overwrite;
  if file deletion is intentional, pass the `--force` equivalent via a full
  rebuild.
