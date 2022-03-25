#!@PYTHON3@
#
# This is part of the Zrythm Manual.
# Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
# See the file index.rst for copying conditions.

# This is a script to remove GFDL translations from the PO
# files when "make gettext" is run for the manual

import os
import subprocess
import re

srcpath = '@MANUAL_SRC_DIR@/locale'
msggrep_bin = '@MSGGREP@'
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
                        '^``(.*)``$',
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
                        '^(.*)$',
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
                        '^:file:`[^`]*`$',
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
