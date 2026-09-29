#!/usr/bin/env python3
# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: FSFAP
"""Inlines external $refs of the form '<file>#/definitions/<name>' in a JSON
schema, producing a self-contained schema (used at build time so schemas can
share definitions from common.schema.json while remaining embeddable and
validatable without external ref resolution).

Transitive dependencies are followed within one source file: internal
'#/definitions/...' refs inside an inlined definition pull in more
definitions from the same file. External refs inside an inlined definition
(referring to a third file) are not followed — with a single shared
common.schema.json this cannot occur; revisit if definitions ever become
spread across multiple files.

Usage: inline_json_schema_refs.py <input.schema.json> <output.schema.json>
"""

import json
import os
import sys

DEFINITIONS_PREFIX = '/definitions/'


def collect_external_refs(node, refs):
    """Collects (file, definition_name) for every external ref in node."""
    if isinstance(node, dict):
        for key, value in node.items():
            if key == '$ref' and isinstance(value, str) and not value.startswith('#'):
                file_part, _, fragment = value.partition('#')
                if not fragment.startswith(DEFINITIONS_PREFIX):
                    raise ValueError(f'Unsupported external $ref: {value}')
                refs.add((file_part, fragment[len(DEFINITIONS_PREFIX):]))
            else:
                collect_external_refs(value, refs)
    elif isinstance(node, list):
        for item in node:
            collect_external_refs(item, refs)


def collect_internal_refs(node, refs):
    """Collects definition names for every internal '#/definitions/...' ref."""
    if isinstance(node, dict):
        for key, value in node.items():
            if key == '$ref' and isinstance(value, str) and value.startswith('#' + DEFINITIONS_PREFIX):
                refs.add(value[len('#' + DEFINITIONS_PREFIX):])
            else:
                collect_internal_refs(value, refs)
    elif isinstance(node, list):
        for item in node:
            collect_internal_refs(item, refs)


def rewrite_external_refs(node):
    """Rewrites external refs to internal ones."""
    if isinstance(node, dict):
        for key, value in node.items():
            if key == '$ref' and isinstance(value, str) and not value.startswith('#'):
                _, _, fragment = value.partition('#')
                name = fragment[len(DEFINITIONS_PREFIX):]
                node[key] = '#' + DEFINITIONS_PREFIX + name
            else:
                rewrite_external_refs(value)
    elif isinstance(node, list):
        for item in node:
            rewrite_external_refs(item)


def main():
    if len(sys.argv) != 3:
        print(f'Usage: {sys.argv[0]} <input.schema.json> <output.schema.json>', file=sys.stderr)
        sys.exit(1)

    input_path, output_path = sys.argv[1], sys.argv[2]
    base_dir = os.path.dirname(input_path)

    with open(input_path) as f:
        schema = json.load(f)

    # Compute the transitive closure of needed definitions
    external_refs = set()
    collect_external_refs(schema, external_refs)

    sources = {}
    needed = set()
    worklist = list(external_refs)
    while worklist:
        file_part, name = worklist.pop()
        if file_part not in sources:
            with open(os.path.join(base_dir, file_part)) as f:
                sources[file_part] = json.load(f)
        source = sources[file_part]
        if name not in source.get('definitions', {}):
            raise ValueError(f'Definition "{name}" not found in {file_part}')
        if (file_part, name) in needed:
            continue
        needed.add((file_part, name))
        internal = set()
        collect_internal_refs(source['definitions'][name], internal)
        for dep in internal:
            worklist.append((file_part, dep))

    # Inline all needed definitions and rewrite external refs to internal;
    # sorted for a deterministic output file (set iteration order varies)
    out_definitions = schema.setdefault('definitions', {})
    for file_part, name in sorted(needed):
        if name in out_definitions:
            raise ValueError(f'Definition name clash: {name} (from {file_part})')
        out_definitions[name] = sources[file_part]['definitions'][name]
    rewrite_external_refs(schema)

    # Every internal ref must resolve: ref rewriting is global, but only
    # definitions reachable from the top-level schema were inlined — an
    # external ref inside an inlined definition would be rewritten to an
    # internal ref with no definition behind it and only fail at runtime
    # validation
    internal_refs = set()
    collect_internal_refs(schema, internal_refs)
    missing = internal_refs - set(schema.get('definitions', {}))
    if missing:
        raise ValueError(
            f'Internal $refs with no inlined definition: {sorted(missing)}')

    with open(output_path, 'w') as f:
        json.dump(schema, f, indent=2)
        f.write('\n')


if __name__ == '__main__':
    main()
