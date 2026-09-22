#!/usr/bin/env python3
# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

"""Fail if qmllint finding counts go up.

Parses qmllint output, filters out known false positives, and compares
the remaining warning and error counts against thresholds that only get
lowered as findings are fixed. Exit code 1 when a threshold is exceeded.

Run in CI after building the all_qmllint target. Lower the thresholds
as findings get fixed; raise them only when a change intentionally
trades one finding for another.
"""

import argparse
import sys

# Known false positives and separately-tracked findings, excluded from
# the counts.
EXCLUDED_PATTERNS = (
    "ZrythmStyle",  # linted by its own target with its own baseline
    'not found on type "QObject"',
    'not found on type "QQuickItem"',
    'Type "QColor" of property "color" not found',
)

DEFAULT_MAX_WARNINGS = 45
DEFAULT_MAX_ERRORS = 16


def filter_findings(lines, path_prefix):
    """Return the Warning/Error lines under path_prefix, minus exclusions."""
    matched = []
    for line in lines:
        if not (line.startswith("Warning: ") or line.startswith("Error: ")):
            continue
        if path_prefix and path_prefix not in line:
            continue
        if any(pattern in line for pattern in EXCLUDED_PATTERNS):
            continue
        matched.append(line)
    return matched


def main(argv=None):
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("output_file", help="qmllint output file, or - for stdin")
    parser.add_argument(
        "--path-prefix",
        default="src/gui/qml/",
        help="only count findings whose file is under this path",
    )
    parser.add_argument(
        "--max-warnings",
        type=int,
        default=DEFAULT_MAX_WARNINGS,
        help="fail when the warning count exceeds this",
    )
    parser.add_argument(
        "--max-errors",
        type=int,
        default=DEFAULT_MAX_ERRORS,
        help="fail when the error count exceeds this",
    )
    args = parser.parse_args(argv)

    if args.output_file == "-":
        lines = sys.stdin.readlines()
    else:
        with open(args.output_file, encoding="utf-8") as f:
            lines = f.readlines()

    findings = filter_findings(lines, args.path_prefix)
    warning_count = sum(1 for line in findings if line.startswith("Warning: "))
    error_count = sum(1 for line in findings if line.startswith("Error: "))

    print(f"QML lint warning count (filtered): {warning_count}")
    print(f"QML lint error count (filtered): {error_count}")

    failed = False
    if warning_count > args.max_warnings:
        print(
            f"Error: QML lint warning count ({warning_count}) exceeds "
            f"threshold ({args.max_warnings})"
        )
        failed = True
    if error_count > args.max_errors:
        print(
            f"Error: QML lint error count ({error_count}) exceeds "
            f"threshold ({args.max_errors})"
        )
        failed = True
    if failed:
        return 1
    print("QML lint check passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
