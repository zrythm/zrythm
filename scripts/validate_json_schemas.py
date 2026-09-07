#!/usr/bin/env python3
# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: FSFAP

"""
Validate JSON schema files.

Requires: pip install jsonschema
"""

import json
import sys
from pathlib import Path

import jsonschema


def main():
    schemas_dir = Path(__file__).parent.parent / 'data' / 'schemas'
    schema_files = list(schemas_dir.glob('*.schema.json'))

    if not schema_files:
        print(f'No schema files found in {schemas_dir}')
        return 0

    print(f'Validating {len(schema_files)} schema file(s)...\n')

    all_valid = True
    for schema_file in sorted(schema_files):
        try:
            with open(schema_file, encoding="utf-8") as f:
                schema = json.load(f)
            jsonschema.Draft7Validator.check_schema(schema)
            print(f'✓ {schema_file.name}')
        except jsonschema.SchemaError as e:
            print(f'✗ {schema_file.name}: {e.message}')
            all_valid = False
        except Exception as e:
            print(f'✗ {schema_file.name}: {e}')
            all_valid = False

    return 0 if all_valid else 1


if __name__ == '__main__':
    sys.exit(main())
