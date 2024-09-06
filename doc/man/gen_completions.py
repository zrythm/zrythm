#!/usr/bin/env python3

# SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#
# This script generates the bash/fish/zsh completion files.

import os
import sys
import subprocess
import shutil
import glob

def main():
    if len(sys.argv) != 5:
        print("Usage: gen_completions.py <manpage> <completions_type> <out_file> <run_sh>")
        sys.exit(1)

    manpage = sys.argv[1]
    completions_type = sys.argv[2]
    out_file = sys.argv[3]
    run_sh = sys.argv[4]

    run_sh_dir = os.path.dirname(run_sh)

    # Change to run_sh directory
    original_dir = os.getcwd()
    os.chdir(run_sh_dir)

    # Run the command
    subprocess.run([run_sh, completions_type, manpage], check=True)

    # Change back to the original directory
    os.chdir(original_dir)

    # Copy the generated completion file
    source_pattern = os.path.join(run_sh_dir, "completions", completions_type, "*zrythm*")
    matching_files = glob.glob(source_pattern)
    
    if matching_files:
        for file in matching_files:
            shutil.copy(file, out_file)
    else:
        print(f"No matching files found for pattern: {source_pattern}")
        sys.exit(1)

if __name__ == "__main__":
    main()