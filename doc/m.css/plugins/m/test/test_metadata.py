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

from m.test import PluginTestCase

class Metadata(PluginTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, '', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'PLUGINS': ['m.htmlsanity', 'm.metadata'],
            'STATIC_PATHS': ['mosra.jpg', 'category.jpg'],
            'FORMATTED_FIELDS': ['summary', 'description', 'badge'],
            'DATE_FORMATS': {'en': ('en_US.UTF-8', '%b %d, %Y')},
            'PAGE_PATHS': ['pages'], # doesn't exist
            'ARTICLE_PATHS': ['articles'],
            'AUTHOR_SAVE_AS': 'author-{slug}.html',
            'AUTHOR_URL': 'author-{slug}.html',
            'CATEGORY_SAVE_AS': 'category-{slug}.html',
            'CATEGORY_URL': 'category-{slug}.html',
            'TAG_SAVE_AS': 'tag-{slug}.html',
            'TAG_URL': 'tag-{slug}.html',
            'M_DISABLE_SOCIAL_META_TAGS': False,
            'M_SHOW_AUTHOR_LIST': True
        })

        self.assertEqual(*self.actual_expected_contents('article.html'))
        self.assertEqual(*self.actual_expected_contents('article-no-image.html'))
        self.assertEqual(*self.actual_expected_contents('article-jumbo.html'))
        self.assertEqual(*self.actual_expected_contents('article-minimal.html'))

        self.assertEqual(*self.actual_expected_contents('author-an-author.html'))
        self.assertEqual(*self.actual_expected_contents('author-author-with-no-image.html'))
        self.assertEqual(*self.actual_expected_contents('author-minimal-author.html'))

        self.assertEqual(*self.actual_expected_contents('category-a-category.html'))
        self.assertEqual(*self.actual_expected_contents('category-category-with-no-image.html'))
        self.assertEqual(*self.actual_expected_contents('category-minimal-category.html'))

        self.assertEqual(*self.actual_expected_contents('tag-a-tag.html'))
        self.assertEqual(*self.actual_expected_contents('tag-minimal-tag.html'))

class TypographyHtmlEscape(PluginTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'typography_html_escape', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'PLUGINS': ['m.htmlsanity', 'm.metadata'],
            'FORMATTED_FIELDS': ['summary', 'description', 'badge'],
            'DATE_FORMATS': {'en': ('en_US.UTF-8', '%b %d, %Y')},
            'PAGE_PATHS': ['pages'], # doesn't exist
            'ARTICLE_PATHS': ['articles'],
            'AUTHOR_SAVE_AS': 'author-{slug}.html',
            'AUTHOR_URL': 'author-{slug}.html',
            'CATEGORY_SAVE_AS': 'category-{slug}.html',
            'CATEGORY_URL': 'category-{slug}.html',
            'TAG_SAVE_AS': 'tag-{slug}.html',
            'TAG_URL': 'tag-{slug}.html',
            'M_DISABLE_SOCIAL_META_TAGS': False,
            'M_SHOW_AUTHOR_LIST': True,
            'M_HTMLSANITY_HYPHENATION': True,
            'M_HTMLSANITY_SMART_QUOTES': True
        })

        self.assertEqual(*self.actual_expected_contents('article.html'))
        self.assertEqual(*self.actual_expected_contents('author-an-author.html'))
        self.assertEqual(*self.actual_expected_contents('category-a-category.html'))
        self.assertEqual(*self.actual_expected_contents('tag-a-tag.html'))
