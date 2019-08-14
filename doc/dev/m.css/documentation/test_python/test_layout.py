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

from . import BaseTestCase

class Layout(BaseTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, '', *args, **kwargs)

    def test(self):
        self.run_python({
            'PROJECT_TITLE': "A project",
            'PROJECT_SUBTITLE': "is cool",

            'INPUT_MODULES': [], # Explicitly none

            'THEME_COLOR': '#00ffff',
            'FAVICON': 'favicon-light.png',
            'PAGE_HEADER': "`A self link <{filename}>`_",
            'FINE_PRINT': "This beautiful thing is done thanks to\n`m.css <https://mcss.mosra.cz>`_.",
            'LINKS_NAVBAR1': [
                ('Pages', 'pages', [
                    ('Getting started', 'getting-started'),
                    ('Troubleshooting', 'troubleshooting')]),
                ('Modules', 'modules', [])],
            'LINKS_NAVBAR2': [
                ('Classes', 'classes', []),
                ('GitHub', 'https://github.com/mosra/m.css', [
                    ('About', 'about')])],
            'SEARCH_DISABLED': False,
            'SEARCH_EXTERNAL_URL': 'https://google.com/search?q=site:mcss.mosra.cz+{}',
            'SEARCH_HELP': "Some *help*.\nOn multiple lines.",
            'HTML_HEADER':
"""<!-- this is extra in the header -->
  <!-- and more, indented -->
<!-- yes. -->""",
            'EXTRA_FILES': ['sitemap.xml']
        })
        self.assertEqual(*self.actual_expected_contents('index.html'))
        self.assertTrue(os.path.exists(os.path.join(self.path, 'output/m-dark+documentation.compiled.css')))
        self.assertTrue(os.path.exists(os.path.join(self.path, 'output/favicon-light.png')))
        self.assertTrue(os.path.exists(os.path.join(self.path, 'output/sitemap.xml')))
