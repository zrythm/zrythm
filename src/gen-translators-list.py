# SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#!/usr/bin/env python3

import io
import sys
import re

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

def elem(a, l):
    return a in l

def main(args):
    if len(args) != 4:
        print("Need 3 arguments")
        sys.exit(-1)

    output_file, output_type, input_file = args[1:]
       
    print(f"Debug: Output file: {output_file}")
    print(f"Debug: Output type: {output_type}")
    print(f"Debug: Input file: {input_file}")

    with open(input_file, 'r', encoding='utf-8') as infile, open(output_file, 'w', encoding='utf-8') as outfile:
        translators = []

        for line in infile:
            matched = re.match(r'\s*\* (.+)', line)
            if matched:
                translator = matched.group(1)
                print(f"Debug: Found translator: {translator}")
                if not elem(translator, translators):
                    translators.append(translator)
                    print(f"Debug: Added new translator: {translator}")

        print(f"Debug: Total translators found: {len(translators)}")

        if output_type == "about":
            print("Debug: Generating 'about' output")
            outfile.write("#define TRANSLATORS_STR \\\n")
            for i, translator in enumerate(translators):
                outfile.write(f'"{translator}')
                if i == len(translators) - 1:
                    outfile.write('"\n')
                else:
                    outfile.write('\\n" \\\n')
                print(f"Debug: Wrote translator {i+1}/{len(translators)}: {translator}")

    print("Debug: Script execution completed")

if __name__ == "__main__":
    main(sys.argv)
