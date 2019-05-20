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

from doxygen import run, default_templates, default_wildcard, default_index_pages

def doxygen_version():
    return subprocess.check_output(['doxygen', '-v']).decode('utf-8').strip()

class BaseTestCase(unittest.TestCase):
    def __init__(self, path, dir, *args, **kwargs):
        unittest.TestCase.__init__(self, *args, **kwargs)
        # Source files for test_something.py are in something_{dir}/ subdirectory
        self.path = os.path.join(os.path.dirname(os.path.realpath(path)), os.path.splitext(os.path.basename(path))[0][5:] + ('_' + dir if dir else ''))

        # Display ALL THE DIFFS
        self.maxDiff = None

    def setUp(self):
        if os.path.exists(os.path.join(self.path, 'html')): shutil.rmtree(os.path.join(self.path, 'html'))

    def run_doxygen(self, templates=default_templates, wildcard=default_wildcard, index_pages=default_index_pages):
        run(os.path.join(self.path, 'Doxyfile'), templates=templates, wildcard=wildcard, index_pages=index_pages, sort_globbed_files=True)

    def actual_expected_contents(self, actual, expected = None):
        if not expected: expected = actual

        with open(os.path.join(self.path, expected)) as f:
            expected_contents = f.read().strip()
        with open(os.path.join(self.path, 'html', actual)) as f:
            actual_contents = f.read().strip()
        return actual_contents, expected_contents

class IntegrationTestCase(BaseTestCase):
    def setUp(self):
        if os.path.exists(os.path.join(self.path, 'xml')): shutil.rmtree(os.path.join(self.path, 'xml'))
        subprocess.run(['doxygen'], cwd=self.path)

        if os.path.exists(os.path.join(self.path, 'html')): shutil.rmtree(os.path.join(self.path, 'html'))
