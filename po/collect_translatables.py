#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#
# This script prints all the translatable files into the given file.

import os
import os.path
import argparse

def main(source_path, build_path, output_path):
    with open(output_path, 'w', encoding='utf-8') as output_file:
        for path in ['inc', 'src', 'resources', 'data']:
            for dirpath, _, filenames in os.walk(path):
                for filename in [
                    f for f in filenames if f.endswith(('.c', '.h', '.cpp', '.ui', '.gschema.xml', '.desktop.in'))]:
                    file_path = os.path.join(dirpath, filename)
                    print(file_path, file=output_file)

        build_path_basename = os.path.basename(build_path)
        print(os.path.join(build_path_basename, 'data', 'org.zrythm.Zrythm.gschema.xml'), file=output_file)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Collect translatable files for Zrythm.")
    parser.add_argument("source_path", help="Path to the source directory")
    parser.add_argument("build_path", help="Path to the build directory")
    parser.add_argument("output_path", help="Path to the output file")
    args = parser.parse_args()

    main(args.source_path, args.build_path, args.output_path)
