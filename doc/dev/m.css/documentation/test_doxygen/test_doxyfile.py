#
#   This file is part of m.css.
#
#   Copyright © 2017, 2018, 2019 Vladimír Vondruš <mosra@centrum.cz>
#
#   Permission is hereby granted, free of charge, to any person obtaining a
#   copy of this software and associated documentation files (the "Software"),
#   to deal in the Software without restriction, including without limitation
#   the rights to use, copy, modify, merge, publish, distribute, sublicense,
#   and/or sell copies of the Software, and to permit persons to whom the
#   Software is furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be included
#   in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#   DEALINGS IN THE SOFTWARE.
#

import os
import shutil
import subprocess
import unittest

from doxygen import parse_doxyfile, State

from . import BaseTestCase

class Doxyfile(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        unittest.TestCase.__init__(self, *args, **kwargs)

        # Display ALL THE DIFFS
        self.maxDiff = None

    def test(self):
        state = State()
        parse_doxyfile(state, 'test_doxygen/doxyfile/Doxyfile')
        self.assertEqual(state.doxyfile, {
            'DOT_FONTNAME': 'Helvetica',
            'DOT_FONTSIZE': 10,
            'HTML_EXTRA_FILES': ['css', 'another.png', 'hello'],
            'HTML_EXTRA_STYLESHEET': ['a.css', 'b.css'],
            'HTML_OUTPUT': 'html',
            'M_CLASS_TREE_EXPAND_LEVELS': 1,
            'M_EXPAND_INNER_TYPES': False,
            'M_FAVICON': 'favicon-dark.png',
            'M_FILE_TREE_EXPAND_LEVELS': 1,
            'M_LINKS_NAVBAR1': ['pages', 'modules'],
            'M_LINKS_NAVBAR2': ['files', 'annotated'], # different order
            'M_MATH_CACHE_FILE': 'm.math.cache',
            'M_PAGE_FINE_PRINT': 'this is "quotes"',
            'M_PAGE_HEADER': 'this is "quotes" \'apostrophes\'',
            'M_SEARCH_DISABLED': False,
            'M_SEARCH_DOWNLOAD_BINARY': False,
            'M_SEARCH_BASE_URL': '',
            'M_SEARCH_EXTERNAL_URL': '',
            'M_SEARCH_HELP':
"""<p class="m-noindent">Search for symbols, directories, files, pages or
modules. You can omit any prefix from the symbol or file path; adding a
<code>:</code> or <code>/</code> suffix lists all members of given symbol or
directory.</p>
<p class="m-noindent">Use <span class="m-label m-dim">&darr;</span>
/ <span class="m-label m-dim">&uarr;</span> to navigate through the list,
<span class="m-label m-dim">Enter</span> to go.
<span class="m-label m-dim">Tab</span> autocompletes common prefix, you can
copy a link to the result using <span class="m-label m-dim">⌘</span>
<span class="m-label m-dim">L</span> while <span class="m-label m-dim">⌘</span>
<span class="m-label m-dim">M</span> produces a Markdown link.</p>
""",
            'M_SHOW_UNDOCUMENTED': False,
            'M_THEME_COLOR': '#22272e',
            'OUTPUT_DIRECTORY': '',
            'PROJECT_BRIEF': 'is cool',
            'PROJECT_NAME': 'My Pet Project',
            'SHOW_INCLUDE_FILES': True,
            'XML_OUTPUT': 'xml'
        })

    def test_subdirs(self):
        state = State()
        with self.assertRaises(NotImplementedError):
            parse_doxyfile(state, 'test_doxygen/doxyfile/Doxyfile-subdirs')

class Upgrade(BaseTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'upgrade_custom_variables', *args, **kwargs)

    def test(self):
        # Copy the Doxyfile to a new location because it gets overwritten
        shutil.copyfile(os.path.join(self.path, 'Doxyfile'),
                        os.path.join(self.path, 'Doxyfile-upgrade'))

        subprocess.run(['doxygen', '-u', 'Doxyfile-upgrade'], cwd=self.path)
        with open(os.path.join(self.path, 'Doxyfile-upgrade'), 'r') as f:
            contents = f.read()

        self.assertFalse('UNKNOWN_VARIABLE' in contents)
        self.assertFalse('COMMENTED_OUT_VARIABLE' in contents)
        self.assertTrue('## HASHED_COMMENTED_VARIABLE = 2' in contents)
        self.assertTrue('##! HASHED_BANG_COMMENTED_VARIABLE = 3 \\' in contents)
        self.assertTrue('##!   HASHED_BANG_COMMENTED_VARIABLE_CONT' in contents)
        self.assertTrue('##!HASHED_BANG_COMMENTED_VARIABLE_NOSPACE = 4' in contents)
        self.assertTrue('INPUT                  = 5' in contents)
        self.assertTrue('##! HASHED_BANG_COMMENTED_VARIABLE_END = 6' in contents)
