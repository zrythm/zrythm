# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

import sys
import pathlib

def file_to_hex_bytes(filepath):
    """Read file and return contents as comma-delimited hex bytes."""
    data = pathlib.Path(filepath).read_bytes()
    return ', '.join(f'0x{b:02x}' for b in data)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <file>", file=sys.stderr)
        sys.exit(1)

    print(file_to_hex_bytes(sys.argv[1]))
