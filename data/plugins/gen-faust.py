#!/usr/bin/env python3
# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

"""Generates self-contained C++ from a .dsp file using the faust CLI.

The generated file is compiled directly into Zrythm (see
src/plugins/faust/faust_base.h for the base classes it builds against).
"""

import argparse
import subprocess
import sys
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(
        description="Generate self-contained faust C++ code for Zrythm"
    )
    parser.add_argument("dsp_file", help="Input .dsp file")
    parser.add_argument("-o", "--output-dir", required=True, help="Output directory")
    parser.add_argument(
        "-a", "--arch", required=True, help="Faust architecture file to use"
    )
    parser.add_argument(
        "-c", "--class-name", required=True, help="Generated class name (-cn)"
    )
    parser.add_argument(
        "--lib",
        action="append",
        default=[],
        help="Extra faust library files to make importable (copied next to the .dsp)",
    )
    parser.add_argument("--faust", default="faust", help="faust executable")
    parser.add_argument(
        "--strip-prefix",
        required=True,
        help=(
            "Absolute path prefix to strip from any paths Faust embeds in the "
            "output (the comment header and the `compile_options` metadata). "
            "Usually the repo root (CMAKE_SOURCE_DIR)."
        ),
    )
    args = parser.parse_args()

    dsp_path = Path(args.dsp_file)
    out_dir = Path(args.output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    # Resolve extra libraries via Faust's import search path (-I) instead of
    # copying them next to the .dsp. All plugin custom commands share the same
    # build directory, so copying to a shared path would race under parallel
    # builds (Ninja): one job could truncate the file while another job's faust
    # is reading it.
    import_dirs: list[str] = []
    for lib in args.lib:
        parent = str(Path(lib).parent.resolve())
        if parent not in import_dirs:
            import_dirs.append(parent)

    out_file = out_dir / f"{args.class_name}.cpp"
    cmd = [args.faust, "-lang", "cpp", "-cn", args.class_name, "-a", args.arch]
    for imp_dir in import_dirs:
        cmd += ["-I", imp_dir]
    cmd += [str(dsp_path), "-o", str(out_file)]
    print("Running:", " ".join(cmd))
    subprocess.run(cmd, check=True)

    # Faust embeds absolute paths (the .dsp input and the -a architecture
    # file) into both the comment header and the `compile_options` metadata
    # string. Strip the supplied prefix (the repo root) so the generated
    # files are reproducible across machines and don't leak developer home
    # directories.
    prefix = str(Path(args.strip_prefix).resolve()) + "/"
    text = out_file.read_text()
    text = text.replace(prefix, "")
    out_file.write_text(text)

    print(f"Generated {out_file}")


if __name__ == "__main__":
    sys.exit(main())
