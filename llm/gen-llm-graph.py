#!/usr/bin/env python3
"""Regenerate graphify-out/llm-graph.json: agent extraction of llm/*.md.
Follows the old graph's schema and style. Every edge is grounded in the
docs' actual content (cross-references or explicit textual overlap)."""
import json
from pathlib import Path

DOCS = [
    ('llm_agent', 'AGENT.md', 'AGENT.md — Algorithm Review Guide', [
        ('reviewworkflow', 'concept', 'Algorithm review workflow (AGENT.md)'),
        ('toolchainmatrix', 'concept', 'Supported toolchain configurations (AGENT.md)'),
        ('thinkandask', 'rationale', 'Think first, ask before assuming (AGENT.md)'),
        ('secondpass', 'concept', 'Second-pass review before reporting done (AGENT.md)'),
    ]),
    ('llm_claude', 'CLAUDE.md', 'CLAUDE.md — Assistant Operating Guide', [
        ('thinkbeforecoding', 'concept', 'Think before coding (CLAUDE.md §1)'),
        ('simplicityfirst', 'concept', 'Simple solutions over clever ones (CLAUDE.md §1)'),
        ('surgicalchanges', 'concept', 'Surgical changes, no scope creep (CLAUDE.md §1)'),
        ('goaldrivenexecution', 'concept', 'Goal-driven execution (CLAUDE.md §4)'),
        ('verifybeforedone', 'concept', 'Verify before claiming done (CLAUDE.md §4)'),
        ('definitionofdone', 'concept', 'Definition of Done checklist (CLAUDE.md §6)'),
        ('autonomyrules', 'concept', 'Autonomy rules: always/ask/never buckets (CLAUDE.md §9)'),
        ('sessionlogs', 'concept', 'Session logs and decision records (CLAUDE.md §0, §6)'),
    ]),
    ('llm_development', 'DEVELOPMENT.md', 'DEVELOPMENT.md — Build & Test Guide', [
        ('buildsystem', 'concept', 'Build system layout (DEVELOPMENT.md §1)'),
        ('ctestfilter', 'concept', 'ctest filtering for algorithm tests (DEVELOPMENT.md §3)'),
        ('snapshotharness', 'concept', 'Snapshot-based test harness (DEVELOPMENT.md §4)'),
        ('formatcheck', 'concept', 'Formatting checks (DEVELOPMENT.md §5)'),
        ('verificationpass', 'concept', 'Verification chain and conditional loops (DEVELOPMENT.md §6)'),
        ('graphifytooling', 'concept', 'Knowledge graph tooling setup (DEVELOPMENT.md §0)'),
    ]),
    ('llm_graphify', 'GRAPHIFY.md', 'GRAPHIFY.md — Knowledge Graph Guide', [
        ('graphworkflow', 'concept', 'Graph workflow: build, update, explain (GRAPHIFY.md)'),
        ('honesty', 'rationale', 'Honesty: EXTRACTED vs INFERRED vs vintage (GRAPHIFY.md)'),
        ('askuserfirst', 'rationale', 'Ask the user before graph builds (GRAPHIFY.md workflow)'),
        ('explainusage', 'concept', 'Explain tool for dependency questions (GRAPHIFY.md)'),
    ]),
    ('llm_performance', 'PERFORMANCE.md', 'PERFORMANCE.md — Performance Guide', [
        ('profilefirst', 'concept', 'Profile before optimizing (PERFORMANCE.md §2)'),
        ('samplingvalidation', 'concept', 'Validate with sampling (PERFORMANCE.md §3)'),
        ('orderofimpact', 'concept', 'Order of impact for optimizations (PERFORMANCE.md §4)'),
        ('memorylayout', 'concept', 'Memory layout optimizations (PERFORMANCE.md §5)'),
        ('parallelization', 'concept', 'Parallelization strategies (PERFORMANCE.md §5)'),
        ('measurementlog', 'concept', 'Measurement log for perf claims (PERFORMANCE.md §5)'),
    ]),
    ('llm_plan', 'PLAN.md', 'PLAN.md — Planning Workflow', [
        ('phaseworkflow', 'concept', 'Phase/todo workflow for huge tasks (PLAN.md §1)'),
        ('perfbranches', 'concept', 'Perf branches and measurement protocol (PLAN.md §5)'),
        ('jointvalidation', 'concept', 'Joint validation of perf work (PLAN.md §5)'),
        ('measurementprotocol', 'concept', 'Measurement protocol (PLAN.md §5)'),
    ]),
    ('llm_rules', 'RULES.md', 'RULES.md — Checkable Development Requirements', [
        ('bindingsrequirements', 'concept', 'Python bindings requirements (RULES.md)'),
        ('patternobjects', 'concept', 'Pattern object requirements (RULES.md)'),
        ('developmentphilosophy', 'rationale', 'Development philosophy (RULES.md)'),
    ]),
]


def doc_node(did, sf, label, subs):
    nodes = [{
        'id': did,
        'label': label,
        'norm_label': label.lower(),
        'file_type': 'document',
        'source_file': sf,
        'source_location': None,
        'source_url': None,
        'captured_at': None,
        'author': None,
        'contributor': None,
        'community': None,
    }]
    for name, ftype, flabel in subs:
        nodes.append({
            'id': f'{did}_{name}',
            'label': flabel,
            'norm_label': flabel.lower(),
            'file_type': ftype,
            'source_file': sf,
            'source_location': None,
            'source_url': None,
            'captured_at': None,
            'author': None,
            'contributor': None,
            'community': None,
        })
    return nodes


def edge(relation, confidence, src, tgt, sf, score=1.0):
    return {
        'relation': relation,
        'confidence': confidence,
        'confidence_score': score,
        'source_file': sf,
        'source_location': None,
        'weight': 1.0,
        'source': src,
        'target': tgt,
    }


def main():
    nodes = []
    refs = []
    for did, sf, label, subs in DOCS:
        nodes += doc_node(did, sf, label, subs)
        for name, _, _ in subs:
            refs.append(edge('references', 'EXTRACTED', did, f'{did}_{name}', sf))

    A = 'llm_agent'; C = 'llm_claude'; D = 'llm_development'; G = 'llm_graphify'
    P = 'llm_performance'; N = 'llm_plan'; R = 'llm_rules'

    refs += [
        edge('references', 'EXTRACTED', C, D, 'CLAUDE.md'),
        edge('references', 'EXTRACTED', C, N, 'CLAUDE.md'),
        edge('references', 'EXTRACTED', C, P, 'CLAUDE.md'),
        edge('references', 'EXTRACTED', C, R, 'CLAUDE.md'),
        edge('references', 'EXTRACTED', C, G, 'CLAUDE.md'),
        edge('references', 'EXTRACTED', C, A, 'CLAUDE.md'),
        edge('references', 'EXTRACTED', A, R, 'AGENT.md'),
        edge('references', 'EXTRACTED', D, N, 'DEVELOPMENT.md'),
        edge('references', 'EXTRACTED', D, P, 'DEVELOPMENT.md'),
        edge('references', 'EXTRACTED', D, G, 'DEVELOPMENT.md'),
        edge('references', 'EXTRACTED', N, P, 'PLAN.md'),
        edge('references', 'EXTRACTED', G, C, 'GRAPHIFY.md'),
        edge('references', 'EXTRACTED', P, N, 'PERFORMANCE.md'),
        edge('conceptually_related_to', 'INFERRED', C, N, 'CLAUDE.md', 0.8),
        edge('conceptually_related_to', 'INFERRED', P, D, 'PERFORMANCE.md', 0.8),
        edge('conceptually_related_to', 'INFERRED', G, P, 'GRAPHIFY.md', 0.7),
        edge('semantically_similar_to', 'INFERRED', f'{C}_definitionofdone', f'{D}_verificationpass', 'CLAUDE.md', 0.7),
        edge('semantically_similar_to', 'INFERRED', f'{N}_measurementprotocol', f'{P}_measurementlog', 'PLAN.md', 0.7),
    ]

    out = {'nodes': nodes, 'links': refs}
    Path('graphify-out/llm-graph.json').write_text(
        json.dumps(out, ensure_ascii=False, indent=1))
    print(f'llm-graph: {len(nodes)} nodes, {len(refs)} links')


if __name__ == '__main__':
    main()
