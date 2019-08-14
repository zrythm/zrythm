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

import unittest

import pelican

from distutils.version import LooseVersion

from test import PageTestCase

class Page(PageTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, '', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_DISABLE_SOCIAL_META_TAGS': False,
            'FORMATTED_FIELDS': ['summary', 'description']
        })

        # The content and summary meta tag shouldn't be there at all
        self.assertEqual(*self.actual_expected_contents('page.html'))

class Minimal(PageTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'minimal', *args, **kwargs)

    def test(self):
        self.run_pelican({
            # There shouldn't be empty meta tags
            'M_DISABLE_SOCIAL_META_TAGS': False,
        })

        # The content and summary meta tag shouldn't be there at all
        self.assertEqual(*self.actual_expected_contents('page.html'))

class Breadcrumb(PageTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'breadcrumb', *args, **kwargs)

    def test(self):
        self.run_pelican({
            # Breadcrumb should not be exposed in social meta tags
            'M_DISABLE_SOCIAL_META_TAGS': False,
        })

        # Internal links should work and guide the user from one page to
        # another
        self.assertEqual(*self.actual_expected_contents('page.html'))
        self.assertEqual(*self.actual_expected_contents('subpage.html'))

class ExtraHtmlHead(PageTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'extra_html_head', *args, **kwargs)

    def test(self):
        self.run_pelican({})

        # The page should contain two extra CSS links, two JS references and a
        # multi-line HTML comment
        self.assertEqual(*self.actual_expected_contents('page.html'))

class HeaderFooter(PageTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'header_footer', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'FORMATTED_FIELDS': ['header', 'footer']
        })

        # The header and footer should have the links expanded
        self.assertEqual(*self.actual_expected_contents('page.html'))

class Landing(PageTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'landing', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'STATIC_PATHS': ['ship.jpg'],
            'FORMATTED_FIELDS': ['landing'],
            # Verify that the image is propagated to social meta tags
            'M_DISABLE_SOCIAL_META_TAGS': False,
        })

        # The landing field should have the links expanded, header should not
        # be shown, footer should be. Navbar brand should be hidden in the
        # second case.
        self.assertEqual(*self.actual_expected_contents('page.html'))
        self.assertEqual(*self.actual_expected_contents('hide-navbar-brand.html'))

class Cover(PageTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'cover', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'STATIC_PATHS': ['ship.jpg'],
            # Verify that the image is propagated to social meta tags
            'M_DISABLE_SOCIAL_META_TAGS': False,
        })

        # Header and footer should be shown and should not break the layout
        self.assertEqual(*self.actual_expected_contents('page.html'))

class TitleSitenameAlias(PageTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'title_sitename_alias', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'SITENAME': "Site name"
        })

        # The page title should be just one name, not both
        self.assertEqual(*self.actual_expected_contents('page.html'))

class HtmlEscape(PageTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'html_escape', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'SITENAME': "<&> in site name",
            'FORMATTED_FIELDS': ['summary', 'description', 'landing', 'header', 'footer'],
            'PAGE_URL': '{slug}.html?and&in&url=""',
            # The social meta tags should be escaped properly as well
            'M_DISABLE_SOCIAL_META_TAGS': False,
            'M_FAVICON': ('favicon.ico?and&in&url=""', 'huh&what')
        })

        # Verify that everything is properly escaped everywhere. The landing
        # page has the name same as site name and the title should contain just
        # one.
        self.assertEqual(*self.actual_expected_contents('page.html'))
        self.assertEqual(*self.actual_expected_contents('landing.html'))
        self.assertEqual(*self.actual_expected_contents('breadcrumb.html'))

    @unittest.skipUnless(LooseVersion(pelican.__version__) > LooseVersion("4.0.1"),
                         "https://github.com/getpelican/pelican/pull/2260")
    def test_content(self):
        self.run_pelican({
            'SITENAME': "<&> in site name",
            'FORMATTED_FIELDS': ['summary', 'description', 'landing', 'header', 'footer'],
            'PAGE_URL': '{slug}.html?and&in&url=""',
            # The social meta tags should be escaped properly as well
            'M_DISABLE_SOCIAL_META_TAGS': False,
            'M_FAVICON': ('favicon.ico?and&in&url=""', 'huh&what')
        })

        # Verify that also the Pelican-produced content has correctly escaped
        # everything.
        self.assertEqual(*self.actual_expected_contents('content.html'))

class GlobalSocialMeta(PageTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'global_social_meta', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_BLOG_DESCRIPTION': 'This is not displayed anywhere.',
            'M_SOCIAL_TWITTER_SITE': '@czmosra',
            'M_SOCIAL_TWITTER_SITE_ID': '1537427036',
            'M_SOCIAL_IMAGE': 'http://your.brand/static/site.png',
            'M_SOCIAL_BLOG_SUMMARY': 'This is also not displayed anywhere.',
            'M_DISABLE_SOCIAL_META_TAGS': False,
        })

        # Verify that the social meta tags are present
        self.assertEqual(*self.actual_expected_contents('page.html'))

class Passthrough(PageTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'passthrough', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'READERS': {}, # re-enable the HTML reader
            'PAGE_PATHS': ['input']
        })

        self.assertEqual(*self.actual_expected_contents('page.html'))
