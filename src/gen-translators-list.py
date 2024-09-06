# SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#!/usr/bin/env python3

import sys
import re

def elem(a, l):
    return a in l

def main(args):
    if len(args) != 4:
        print("Need 3 arguments")
        sys.exit(-1)

    output_file, output_type, input_file = args[1:]

    with open(input_file, 'r') as infile, open(output_file, 'w') as outfile:
        translators = []

        for line in infile:
            matched = re.match(r'\s*\* (.+)', line)
            if matched:
                translator = matched.group(1)
                if not elem(translator, translators):
                    translators.append(translator)

        if output_type == "about":
            outfile.write("#define TRANSLATORS_STR \\\n")
            for i, translator in enumerate(translators):
                outfile.write(f'"{translator}')
                if i == len(translators) - 1:
                    outfile.write('"\n')
                else:
                    outfile.write('\\n" \\\n')

if __name__ == "__main__":
    main(sys.argv)
