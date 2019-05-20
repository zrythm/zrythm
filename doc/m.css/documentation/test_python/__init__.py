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

import copy
import sys
import os
import inspect
import shutil
import unittest

from python import run, default_templates, default_config

class BaseTestCase(unittest.TestCase):
    def __init__(self, path, dir, *args, **kwargs):
        unittest.TestCase.__init__(self, *args, **kwargs)
        # Full directory name (for test_something.py the directory is something_dir{}
        self.dirname = os.path.splitext(os.path.basename(path))[0][5:] + ('_' + dir if dir else '')
        # Absolute path to this directory
        self.path = os.path.join(os.path.dirname(os.path.realpath(path)), self.dirname)

        # Display ALL THE DIFFS
        self.maxDiff = None

    def setUp(self):
        if os.path.exists(os.path.join(self.path, 'output')): shutil.rmtree(os.path.join(self.path, 'output'))

    def run_python(self, config_overrides={}, templates=default_templates):
        # Defaults that make sense for the tests
        config = copy.deepcopy(default_config)
        config.update({
            'FINE_PRINT': None,
            'THEME_COLOR': None,
            'FAVICON': None,
            'LINKS_NAVBAR1': None,
            'LINKS_NAVBAR2': None,
            # None instead of [] so we can detect even an empty override
            'INPUT_MODULES': None,
            'SEARCH_DISABLED': True,
            'OUTPUT': os.path.join(self.path, 'output')
        })

        # Update it with config overrides, specify the input module if not
        # already
        config.update(config_overrides)
        if config['INPUT_MODULES'] is None:
            sys.path.append(self.path)
            config['INPUT_MODULES'] = [self.dirname]

        run(self.path, config, templates=templates)

    def actual_expected_contents(self, actual, expected = None):
        if not expected: expected = actual

        with open(os.path.join(self.path, expected)) as f:
            expected_contents = f.read().strip()
        with open(os.path.join(self.path, 'output', actual)) as f:
            actual_contents = f.read().strip()
        return actual_contents, expected_contents
