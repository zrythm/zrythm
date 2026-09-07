# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

"""Embed a file's contents as a hex byte array into a C++ header template.

The template's placeholder line (e.g. `@SCHEMA_JSON_HEX_BYTES@`) is
replaced with the comma-delimited hex bytes of the input file.

Usage: generate_embedded_header.py <input-file> <template> <output> <placeholder>
"""

import pathlib
import sys


def main() -> None:
    if len(sys.argv) != 5:
        print(
            f"Usage: {sys.argv[0]} <input-file> <template> <output> <placeholder>",
            file=sys.stderr,
        )
        sys.exit(1)

    input_file, template_file, output_file, placeholder = sys.argv[1:]

    data = pathlib.Path(input_file).read_bytes()
    hex_bytes = ", ".join(f"0x{b:02x}" for b in data)

    template = pathlib.Path(template_file).read_text()
    if placeholder not in template:
        print(
            f"Placeholder {placeholder} not found in {template_file}",
            file=sys.stderr,
        )
        sys.exit(1)

    pathlib.Path(output_file).write_text(template.replace(placeholder, hex_bytes))


if __name__ == "__main__":
    main()
