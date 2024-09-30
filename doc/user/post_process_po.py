#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: FSFAP

# This is a script to remove GFDL translations from the PO
# files when "make gettext" is run for the manual

import os
import subprocess
import re
import argparse

def main(srcpath, msggrep_bin):
    for subdir, dirs, files in os.walk(srcpath):
        for file in files:
            fullpath = os.path.join(subdir,file)
            if file.endswith('.po'):
                print ('processing file %s' % fullpath)
                with open(fullpath, 'r') as f :
                    filedata = f.read()

                    # remove absolute directories (possible
                    # bug with new versions of sphinx-intl)
                    filedata = re.sub(
                        r'#: .*?/doc/user/',
                        '#: ../../', filedata, flags=re.DOTALL)

                    # run msggrep
                    if file.endswith ('zrythm-manual.po'):

                        # remove prolog
                        completed_process = subprocess.run ([
                            msggrep_bin,
                            '--invert-match',
                            '--location',
                            '../../../../<rst_prolog>',
                            '-',
                            ],
                            input = filedata,
                            text = True,
                            capture_output = True,
                            check = True)
                        filedata = completed_process.stdout

                        # remove scheme code
                        completed_process = subprocess.run ([
                            msggrep_bin,
                            '--invert-match',
                            '--msgid',
                            '--regexp',
                            '^``(.*)``',
                            '-',
                            ],
                            input = filedata,
                            text = True,
                            capture_output = True,
                            check = True)
                        filedata = completed_process.stdout
                        completed_process = subprocess.run ([
                            msggrep_bin,
                            '--invert-match',
                            '--msgid',
                            '--regexp',
                            '^(.*)',
                            '-',
                            ],
                            input = filedata,
                            text = True,
                            capture_output = True,
                            check = True)
                        filedata = completed_process.stdout

                        # remove :file:`<path>`
                        completed_process = subprocess.run ([
                            msggrep_bin,
                            '--invert-match',
                            '--msgid',
                            '--regexp',
                            '^:file:`[^`]*`',
                            '-',
                            ],
                            input = filedata,
                            text = True,
                            capture_output = True,
                            check = True)
                        filedata = completed_process.stdout

                        # remove "Permission is granted"
                        completed_process = subprocess.run ([
                            msggrep_bin,
                            '--invert-match',
                            '--msgid',
                            '--regexp',
                            'Permission is granted to copy',
                            '-',
                            ],
                            input = filedata,
                            text = True,
                            capture_output = True,
                            check = True)
                        filedata = completed_process.stdout

                        # remove "Copyright ©"
                        completed_process = subprocess.run ([
                            msggrep_bin,
                            '--invert-match',
                            '--msgid',
                            '--regexp',
                            'Copyright ©',
                            '-',
                            ],
                            input = filedata,
                            text = True,
                            capture_output = True,
                            check = True)
                        filedata = completed_process.stdout

                    with open(fullpath, 'w') as f:
                        print ('writing to file %s' % fullpath)
                        f.write(filedata)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Process PO files.')
    parser.add_argument('srcpath', help='Path to the source directory')
    parser.add_argument('msggrep_bin', help='Path to the msggrep binary')
    args = parser.parse_args()

    main(args.srcpath, args.msggrep_bin)
