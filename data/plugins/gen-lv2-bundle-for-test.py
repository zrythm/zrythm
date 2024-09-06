#!/usr/bin/env python3
# SPDX-FileCopyrightText: © 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#
# This script copies the produced .ttls and shared module to
# the given output dir (used in tests).

import os
import sys
import shutil

def main():
    # Check if correct number of arguments are provided
    if len(sys.argv) != 5:
        print("Usage: python3 gen-lv2-bundle-for-test.py <out_dir> <pl_ttl> <pl_manifest_ttl> <pl_module>")
        sys.exit(1)

    out_dir = sys.argv[1]
    pl_ttl = sys.argv[2]
    pl_manifest_ttl = sys.argv[3]
    pl_module = sys.argv[4]

    # Remove output directory if it exists
    if os.path.exists(out_dir):
        shutil.rmtree(out_dir)

    # Create output directory
    os.makedirs(out_dir)

    # Copy files to output directory
    shutil.copy(pl_ttl, out_dir)
    shutil.copy(pl_module, out_dir)
    shutil.copy(pl_manifest_ttl, os.path.join(out_dir, "manifest.ttl"))

if __name__ == "__main__":
    main()