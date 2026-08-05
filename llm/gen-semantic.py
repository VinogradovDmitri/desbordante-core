#!/usr/bin/env python3
"""Generate graphify-out/src-semantic.json: restore CMake-target nodes and
conceptual edges that the AST extraction misses (PUBLIC links, aggregator
libraries, bindings). Ids use the final merged form (repo::...) so the
result injects cleanly into graph.json after merge-graphs.
"""
import json
import re
from pathlib import Path

FILES = (list(Path('src').rglob('CMakeLists.txt'))
         + [Path('src/tests/benchmark/CMakeLists.txt'),
            Path('src/python_bindings/CMakeLists.txt')])

NAME_RE = re.compile(r'\bset\s*\(\s*NAME\s+([\w.]+)\s*\)')
MODNAME_RE = re.compile(r'\bset\s*\(\s*MODULE_NAME\s+"?([\w.]+)')
MODULE_RE = re.compile(r'\bpybind11_add_module\s*\(\s*([\w.]+)')
LINK_RE = re.compile(r'target_link_libraries\s*\(')
LIB_RE = re.compile(r'(?:\$\{DESBORDANTE_PREFIX\}::|Desbordante::|Desbordante\.)((?:bindlib\.)?[\w.:]+)')


def read_balanced(text, start, depth=1):
    i = start
    while i < len(text):
        c = text[i]
        if c == '(':
            depth += 1
        elif c == ')':
            depth -= 1
            if depth == 0:
                return text[start:i], i + 1
        i += 1
    return text[start:], len(text)


def norm_name(raw):
    return raw.replace('::', '.')


def node_id(dir_rel, name):
    return 'repo::src_' + dir_rel.replace('/', '_') + '_cmakelists_' + name.replace('.', '_')


def collect_targets():
    targets = {}
    for f in FILES:
        text = f.read_text(errors='ignore')
        names = set(NAME_RE.findall(text)) | set(MODULE_RE.findall(text)) | set(MODNAME_RE.findall(text))
        for n in names:
            targets.setdefault(n, (f, f.parent))
    return targets


def section_targets(text):
    anchors = []
    for m in NAME_RE.finditer(text):
        anchors.append((m.start(), m.group(1)))
    for m in MODNAME_RE.finditer(text):
        anchors.append((m.start(), m.group(1)))
    for m in MODULE_RE.finditer(text):
        anchors.append((m.start(), m.group(1)))
    anchors.sort()
    return anchors


def block_parts(text, m):
    body, _ = read_balanced(text, m.end())
    toks = body.split(None, 1)
    if not toks:
        return None, ''
    return toks[0], (body[len(toks[0]):] if len(toks) == 2 else '')


def block_edges(rest, this_target, targets):
    for lib in LIB_RE.findall(rest):
        lib = norm_name(lib)
        if lib in targets and lib != this_target:
            yield (this_target, lib)


def main():
    targets = collect_targets()
    edges = set()
    for f in FILES:
        text = f.read_text(errors='ignore')
        anchors = section_targets(text)
        module_names = [n for _, n in anchors if n == 'desbordante']
        for block in LINK_RE.finditer(text):
            ref, rest = block_parts(text, block)
            if ref is None:
                continue
            if ref.startswith('${'):
                ref = ref[2:-1]
                if ref == 'MODULE_NAME':
                    if not module_names:
                        continue
                    this_target = module_names[0]
                else:
                    this_target = None
                    for pos, name in anchors:
                        if block.start() >= pos:
                            this_target = name
                        else:
                            break
            else:
                this_target = norm_name(ref)
            if this_target is None or this_target not in targets or targets[this_target][0] != f:
                continue
            edges |= set(block_edges(rest, this_target, targets))

    nodes, node_ids = [], set()
    for name, (f, parent) in sorted(targets.items()):
        rel = str(parent.relative_to('.'))
        if rel.startswith('src/'):
            rel = rel[len('src/'):]
        nid = node_id(rel, name)
        if name == 'desbordante':
            nid = 'repo::desbordante'
        if nid in node_ids:
            continue
        node_ids.add(nid)
        label = KNOWN.get(name, f'{name} — CMake library')
        nodes.append({
            'id': nid,
            'label': label,
            'norm_label': label.lower(),
            'file_type': 'code',
            'source_file': str(f.relative_to('.')),
            'source_location': None,
            'source_url': None,
            'captured_at': None,
            'author': None,
            'contributor': None,
            'community': None,
            'repo': 'repo',
            'local_id': nid.split('::', 1)[1],
        })

    def full_id(name):
        if name == 'desbordante':
            return 'repo::desbordante'
        rel = str(targets[name][1].relative_to('.'))
        if rel.startswith('src/'):
            rel = rel[len('src/'):]
        return node_id(rel, name)

    links, seen = [], set()
    for src, tgt in sorted(edges):
        src_id, tgt_id = full_id(src), full_id(tgt)
        key = (src_id, tgt_id)
        if key in seen:
            continue
        seen.add(key)
        links.append({
            'relation': 'conceptually_related_to',
            'confidence': 'EXTRACTED',
            'confidence_score': 1.0,
            'source_file': str(targets[src][0].relative_to('.')),
            'source_location': None,
            'weight': 1.0,
            '_origin': 'agent-semantic',
            'source': src_id,
            'target': tgt_id,
        })
    Path('graphify-out/src-semantic.json').write_text(
        json.dumps({'nodes': nodes, 'links': links}, ensure_ascii=False, indent=1))
    print(f'targets: {len(targets)} | nodes: {len(nodes)} | edges: {len(links)}')
    for probe in ('create_algo', 'config', 'algos', 'desbordante'):
        hits = [n['id'] for n in nodes if n['id'].endswith(probe) or n['id'] == 'repo::desbordante']
        print(' ', probe, '->', hits)


KNOWN = {
    'create_algo': 'create_algo — algorithm library aggregator (INTERFACE)',
    'algos': 'algos — algorithm framework library',
    'config': 'config — configuration options library',
    'desbordante': 'desbordante — Python bindings module (pybind11)',
    'bindlib.util': 'bindlib.util — Python bindings utility library',
    'bindlib.data': 'bindlib.data — Python bindings data library',
}

if __name__ == '__main__':
    main()
