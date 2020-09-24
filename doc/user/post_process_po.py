#!@PYTHON3@
#
# This is part of the Zrythm Manual.
# Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
# See the file index.rst for copying conditions.

# This is a script to remove GFDL translations from the PO
# files when "make gettext" is run for the manual

import os
import re

srcpath = '@MANUAL_SRC_DIR@/locale'
for subdir, dirs, files in os.walk(srcpath):
    for file in files:
        fullpath = os.path.join(subdir,file)
        if file.endswith('scripting.po'):
            with open(fullpath, 'r') as f :
                filedata = f.read()

                # remove "Schema Procedure"
                # translations
                filedata = re.sub(
                        r'#:.*?api.*?\n(#: .*?\n)*msgid "Scheme Procedure.*?\nmsgstr .*?\n\n',
                    '', filedata)
                filedata = re.sub(
                    r'#:.*?api.*?\n(#: .*?\n)*msgid ""\n"Scheme Procedure.*?\n(".*"\n)*?msgstr .*?\n\n',
                    '', filedata)

                filedata = re.sub(
                    r'#:.*?api.*?:3\n.*?\nmsgstr .*?\n\n',
                    '', filedata)

                # remove absolute directories (possible
                # bug with new versions of sphinx-intl)
                filedata = re.sub(
                    r'#: .*?/doc/user/',
                    '#: ../../', filedata, flags=re.DOTALL)

                with open(fullpath, 'w') as fw:
                  fw.write(filedata)

        elif file.endswith('appendix.po'):
            with open(fullpath, 'r') as f :
                filedata = f.read()

                # remove GFDL translations
                filedata = re.sub(
                    r'#:.*?gnu-free-documentation-license.*?msgstr ""\n',
                    '', filedata, flags=re.DOTALL)

                # clean extra new lines
                filedata = re.sub(
                    r'\n{2,600}',
                    '\n\n', filedata, flags=re.DOTALL)

                # remove absolute directories (possible
                # bug with new versions of sphinx-intl)
                filedata = re.sub(
                    r'#: .*?/doc/user/',
                    '#: ../../', filedata, flags=re.DOTALL)

                with open(fullpath, 'w') as fw:
                  fw.write(filedata)
        elif file.endswith('.po'):
            with open(fullpath, 'r') as f :
                filedata = f.read()

                # remove absolute directories (possible
                # bug with new versions of sphinx-intl)
                filedata = re.sub(
                    r'#: .*?/doc/user/',
                    '#: ../../', filedata, flags=re.DOTALL)

                with open(fullpath, 'w') as f:
                  f.write(filedata)
