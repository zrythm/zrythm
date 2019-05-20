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
import unittest

from pelican import read_settings, Pelican

class PluginTestCase(unittest.TestCase):
    def __init__(self, path, dir, *args, **kwargs):
        unittest.TestCase.__init__(self, *args, **kwargs)
        # Source files for test_something.py are in something_{dir}/ subdirectory
        self.path = os.path.join(os.path.dirname(os.path.realpath(path)), os.path.splitext(os.path.basename(path))[0][5:] + ('_' + dir if dir else ''))

        # Display ALL THE DIFFS
        self.maxDiff = None

    def setUp(self):
        if os.path.exists(os.path.join(self.path, 'output')): shutil.rmtree(os.path.join(self.path, 'output'))

    def run_pelican(self, settings):
        implicit_settings = {
            # Contains just stuff that isn't required by the m.css theme itself,
            # but is needed to have the test setup working correctly
            'RELATIVE_URLS': True,
            'TIMEZONE': 'UTC',
            'READERS': {'html': None},
            'SITEURL': '.',
            'PATH': os.path.join(self.path),
            'OUTPUT_PATH': os.path.join(self.path, 'output'),
            'PAGE_PATHS': [''],
            'PAGE_SAVE_AS': '{slug}.html',
            'PAGE_URL': '{slug}.html',
            'PAGE_EXCLUDES': ['output'],
            'ARTICLE_PATHS': ['articles'], # does not exist
            # Don't render feeds, we don't want to test them all the time
            'FEED_ALL_ATOM': None,
            'CATEGORY_FEED_ATOM': None,
            'THEME': '../pelican-theme',
            'PLUGIN_PATHS': ['.'],
            'THEME_STATIC_DIR': 'static',
            'M_CSS_FILES': ['https://fonts.googleapis.com/css?family=Source+Sans+Pro:400,400i,600,600i',
               'static/m-dark.css'],
            'M_FINE_PRINT': None,
            'M_DISABLE_SOCIAL_META_TAGS': True,
            'DIRECT_TEMPLATES': [],
            'SLUGIFY_SOURCE': 'basename'
        }
        implicit_settings.update(settings)
        settings = read_settings(path=None, override=implicit_settings)
        pelican = Pelican(settings=settings)
        pelican.run()

    def actual_expected_contents(self, actual, expected = None):
        if not expected: expected = actual

        with open(os.path.join(self.path, expected)) as f:
            expected_contents = f.read().strip()
        with open(os.path.join(self.path, 'output', actual)) as f:
            actual_contents = f.read().strip()
        return actual_contents, expected_contents
