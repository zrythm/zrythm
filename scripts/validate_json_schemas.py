#!/usr/bin/env python3
# SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: FSFAP

"""
Validate JSON schema files and any accompanying valid/invalid fixtures.

Requires: pip install jsonschema
"""

import json
import sys
from pathlib import Path

import jsonschema


def validate_fixtures(schema_file, schema, repo_root):
    schema_name = schema_file.name.removesuffix('.schema.json')
    fixtures_dir = (
        repo_root / 'tests' / 'fixtures' / 'schemas' / schema_name)
    if not fixtures_dir.exists():
        return True

    validator = jsonschema.Draft7Validator(schema)
    all_valid = True
    counts = {True: 0, False: 0}
    for expected_valid in (True, False):
        expectation_dir = fixtures_dir / ('valid' if expected_valid else 'invalid')
        fixture_files = sorted(expectation_dir.glob('*.json'))
        if not fixture_files:
            print(f'✗ No fixtures found in {expectation_dir.relative_to(repo_root)}')
            all_valid = False
            continue
        for fixture_file in fixture_files:
            fixture_path = fixture_file.relative_to(repo_root)
            try:
                with open(fixture_file, encoding="utf-8") as f:
                    document = json.load(f)
                errors = list(validator.iter_errors(document))
                if expected_valid and errors:
                    print(f'✗ {fixture_path}: {errors[0].message}')
                    all_valid = False
                elif not expected_valid and not errors:
                    print(f'✗ {fixture_path}: schema accepted an invalid fixture')
                    all_valid = False
                else:
                    counts[expected_valid] += 1
            except Exception as e:
                print(f'✗ {fixture_path}: {e}')
                all_valid = False

    if all_valid:
        print(
            f'✓ {schema_file.name} fixtures '
            f'({counts[True]} valid, {counts[False]} invalid)')
    return all_valid


def main():
    repo_root = Path(__file__).parent.parent
    schemas_dir = repo_root / 'data' / 'schemas'
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
            if not validate_fixtures(schema_file, schema, repo_root):
                all_valid = False
        except jsonschema.SchemaError as e:
            print(f'✗ {schema_file.name}: {e.message}')
            all_valid = False
        except Exception as e:
            print(f'✗ {schema_file.name}: {e}')
            all_valid = False

    return 0 if all_valid else 1


if __name__ == '__main__':
    sys.exit(main())
