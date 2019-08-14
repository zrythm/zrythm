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

from test import MinimalTestCase, BaseTestCase

class Layout(BaseTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, '', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'SITENAME': 'Your Brand',
            'M_BLOG_NAME': 'Your Brand Blog',
            'M_SITE_LOGO_TEXT': 'Your.brand',
            'M_FAVICON': ('favicon.ico', 'image/x-icon'),
            'M_LINKS_NAVBAR1': [
                ('Features', 'features.html', 'features', []),
                ('Showcase', '#', 'showcase', [
                    ('Requirements', 'showcase-requirements.html', 'showcase-requirements'),
                    ('Live demo', 'http://demo.your.brand/', ''),
                    ('Get a quote', 'mailto:you@your.brand', '')]),
                ('Download', '#', 'download', [])],
            'M_LINKS_NAVBAR2': [('Blog', 'archives.html', '[blog]', [
                    ('News', '#', ''),
                    ('Archive', '#', ''),
                    ('Write a guest post', 'guest-post-howto.html', 'guest-post-howto')]),
                ('Contact', '#', 'contact', [])],
            'M_LINKS_FOOTER1': [('Your Brand', 'index.html'),
                   ('Mission', '#'),
                   ('', ''),
                   ('The People', '#')],
            'M_LINKS_FOOTER2':[('Features', 'features.html'),
                   ('', ''),
                   ('Live demo', 'http://demo.your.brand/'),
                   ('Requirements', 'showcase-requirements.html')],
            'M_LINKS_FOOTER3': [('Download', ''),
                   ('Packages', '#'),
                   ('', ''),
                   ('Source', '#')],
            'M_LINKS_FOOTER4':[('Contact', ''),
                   ('E-mail', 'mailto:you@your.brand'),
                   ('', ''),
                   ('GitHub', 'https://github.com/your-brand')],
            # multiline to test indentation
            'M_FINE_PRINT': """
Your Brand. Copyright © `You <mailto:you@your.brand>`_, 2017.
All rights reserved.
""",
            'PAGE_PATHS': ['.'],
            'PAGE_SAVE_AS': '{slug}.html',
            'PAGE_URL': '{slug}.html',
            'ARTICLE_PATHS': ['articles'] # doesn't exist
        })

        # - page title is M_BLOG_NAME for index, as it is a blog page
        # - the Blog entry should be highlighted
        # - mailto and external URL links should not have SITEURL prepended in
        #   both top navbar and bottom navigation
        # - footer spacers should put &nbsp; there
        # - again, the archives page and index page should be exactly the same
        self.assertEqual(*self.actual_expected_contents('index.html'))
        self.assertEqual(*self.actual_expected_contents('archives.html', 'index.html'))

        # - page title is SITENAME, as these are pages
        # - corresponding items in top navbar should be highlighted
        self.assertEqual(*self.actual_expected_contents('features.html'))
        self.assertEqual(*self.actual_expected_contents('showcase-requirements.html'))
        self.assertEqual(*self.actual_expected_contents('guest-post-howto.html'))

class Minimal(MinimalTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'minimal', *args, **kwargs)

    def test(self):
        self.run_pelican({
            # This is the minimal that's required. Not even the M_THEME_COLOR
            # is required.
            'THEME': '.',
            'PLUGIN_PATHS': ['../plugins'],
            'PLUGINS': ['m.htmlsanity'],
            'THEME_STATIC_DIR': 'static',
            'M_CSS_FILES': ['https://fonts.googleapis.com/css?family=Source+Sans+Pro:400,400i,600,600i',
               'static/m-dark.css'],
            'M_THEME_COLOR': '#22272e'})

        # The archives and index page should be exactly the same
        self.assertEqual(*self.actual_expected_contents('index.html'))
        self.assertEqual(*self.actual_expected_contents('archives.html', 'index.html'))

        # Verify that we're *really* using the default setup and not disabling
        # any pages -- but don't verify the page content, as these are not
        # supported anyway
        self.assertTrue(os.path.exists(os.path.join(self.path, 'output/authors.html')))
        self.assertTrue(os.path.exists(os.path.join(self.path, 'output/categories.html')))
        self.assertTrue(os.path.exists(os.path.join(self.path, 'output/tags.html')))

        # The CSS files should be copied along
        self.assertTrue(os.path.exists(os.path.join(self.path, 'output/static/m-grid.css')))
        self.assertTrue(os.path.exists(os.path.join(self.path, 'output/static/m-components.css')))
        self.assertTrue(os.path.exists(os.path.join(self.path, 'output/static/m-dark.css')))
        self.assertTrue(os.path.exists(os.path.join(self.path, 'output/static/pygments-dark.css')))

class OneColumnNavbar(BaseTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'one_column_navbar', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_DISABLE_SOCIAL_META_TAGS': True,
            'M_LINKS_NAVBAR1': [
                ('Features', '#', 'features', []),
                ('A long item caption that really should not wrap on small screen', '#', '', []),
                ('Blog', 'archives.html', '[blog]', [])],
            'M_FINE_PRINT': None
        })

        # The navbar should be full 12 columns
        self.assertEqual(*self.actual_expected_contents('index.html'))

class NoFooter(BaseTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'no_footer', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_DISABLE_SOCIAL_META_TAGS': True,
            'M_FINE_PRINT': None
        })

        # There should be no footer at all
        self.assertEqual(*self.actual_expected_contents('index.html'))

class DisableFinePrint(BaseTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'disable_fine_print', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_DISABLE_SOCIAL_META_TAGS': True,
            'M_LINKS_FOOTER1': [('Your Brand', 'index.html')],
            'M_FINE_PRINT': None
        })

        # There should be footer with just the first and last column and no
        # fine print
        self.assertEqual(*self.actual_expected_contents('index.html'))

class DisableBlogLinks(BaseTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'disable_blog_links', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_DISABLE_SOCIAL_META_TAGS': True,
            'M_LINKS_FOOTER1': [('Your Brand', 'index.html')],
            'M_LINKS_FOOTER4': None,
        })

        # There should be just the first column
        self.assertEqual(*self.actual_expected_contents('index.html'))

class HtmlEscape(BaseTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'html_escape', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'SITENAME': 'A <&> site',
            'M_BLOG_NAME': 'A <&> blog',
            'M_SITE_LOGO': 'image.png?and&in&url=""',
            'M_SITE_LOGO_TEXT': '<&>',
            'M_FAVICON': ('favicon.ico?and&in&url=""', 'huh&what'),
            'M_LINKS_NAVBAR1': [
                ('An <&> item', 'item.html?and&in&url=""', '', [
                    ('A <&> subitem', 'sub.html?and&in&url=""', '')]),
                ('Another <&> item', 'item.html?and&in&url=""', '', [])],
            'M_LINKS_NAVBAR2': [
                ('An <&> item', 'item.html?and&in&url=""', '', [
                    ('A <&> subitem', 'sub.html?and&in&url=""', '')]),
                ('Another <&> item', 'item.html?and&in&url=""', '', [])],
            'M_LINKS_FOOTER1': [
                ('An <&> item', 'item.html?and&in&url=""'),
                ('A <&> subitem', 'sub.html?and&in&url=""')],
            'M_LINKS_FOOTER2': [
                ('An <&> item', 'item.html?and&in&url=""'),
                ('A <&> subitem', 'sub.html?and&in&url=""')],
            'M_LINKS_FOOTER3': [
                ('An <&> item', 'item.html?and&in&url=""'),
                ('A <&> subitem', 'sub.html?and&in&url=""')],
            'M_LINKS_FOOTER4': [
                ('An <&> item', 'item.html?and&in&url=""'),
                ('A <&> subitem', 'sub.html?and&in&url=""')],
            'M_FINE_PRINT': 'An <&> in fine print.',
            'CATEGORY_URL': 'tag-{slug}.html?and&in&url=""'
        })

        # Verify that everything is properly escaped everywhere
        self.assertEqual(*self.actual_expected_contents('index.html'))
        self.assertEqual(*self.actual_expected_contents('archives.html', 'index.html'))

class GlobalSocialMeta(BaseTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'global_social_meta', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'SITEURL': 'http://your.brand',
            'M_BLOG_NAME': 'Your Brand Blog',
            'M_BLOG_URL': 'http://blog.your.brand/',
            'M_BLOG_DESCRIPTION': 'Your Brand provides everything you\'ll ever need.',
            'M_SOCIAL_TWITTER_SITE': '@czmosra',
            'M_SOCIAL_TWITTER_SITE_ID': '1537427036',
            'M_SOCIAL_IMAGE': 'http://your.brand/static/site.png',
            'M_SOCIAL_BLOG_SUMMARY': 'This is The Brand.'
        })

        # Verify that the social meta tags are present. Archives should have a
        # different og:url but nothing else.
        self.assertEqual(*self.actual_expected_contents('index.html'))
        self.assertEqual(*self.actual_expected_contents('archives.html'))
