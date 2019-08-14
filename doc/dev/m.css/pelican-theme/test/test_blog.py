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

from test import BlogTestCase

class Blog(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, '', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_DISABLE_SOCIAL_META_TAGS': False,
            'AUTHOR': "Implicit Author",
            'STATIC_PATHS': ['ship.jpg'],
            'M_BLOG_URL': 'archives.html',
            'M_FAVICON': ('favicon.png', 'image/png'),
            'M_BLOG_FAVICON': ('favicon-blog.ico', 'image/x-icon'),
            'FORMATTED_FIELDS': ['summary', 'description']
        })

        # The archives and index page should be exactly the same, except for
        # og:url that points to the file itself
        self.assertEqual(*self.actual_expected_contents('index.html'))
        self.assertEqual(*self.actual_expected_contents('archives.html'))

        # Default and jumbo article rendering
        self.assertEqual(*self.actual_expected_contents('article.html'))
        self.assertEqual(*self.actual_expected_contents('article-jumbo.html'))

        # Author, category, tag pages (selecting just one of each)
        self.assertEqual(*self.actual_expected_contents('author-explicit-author.html'))
        self.assertEqual(*self.actual_expected_contents('category-another-category.html'))
        self.assertEqual(*self.actual_expected_contents('tag-third.html'))

class Minimal(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'minimal', *args, **kwargs)

    def test(self):
        self.run_pelican({
            # There shouldn't be empty meta tags
            'M_DISABLE_SOCIAL_META_TAGS': False,
        })

        # The summary, content blocks should not be there at all, no mention of
        # authors. Index and archive page should be exactly the same. Header
        # metadata should be empty as well. Verify also the category page as it
        # is a base for author/tag listing as well.
        self.assertEqual(*self.actual_expected_contents('index.html'))
        self.assertEqual(*self.actual_expected_contents('archives.html', 'index.html'))
        self.assertEqual(*self.actual_expected_contents('category-misc.html'))
        self.assertEqual(*self.actual_expected_contents('article.html'))
        self.assertEqual(*self.actual_expected_contents('article-jumbo.html'))

class JumboInverted(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'jumbo_inverted', *args, **kwargs)

    def test(self):
        self.run_pelican({})

        # The <article> should have a m-inverted class
        self.assertEqual(*self.actual_expected_contents('article-jumbo.html'))

class AuthorList(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'author_list', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'AUTHOR': "An Author",
            'M_SHOW_AUTHOR_LIST': True
        })

        # Verify both right sidebar and bottom columns on jumbo articles. It's
        # without tags, which is the third option to be considered there.
        self.assertEqual(*self.actual_expected_contents('index.html'))
        self.assertEqual(*self.actual_expected_contents('article-jumbo.html'))

class FooterLinks(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'footer_links', *args, **kwargs)

    def test(self):
        self.run_pelican({'M_LINKS_FOOTER2': [('Title', '#')]})

        # There should be second and fourth column
        self.assertEqual(*self.actual_expected_contents('index.html'))

class ModifiedDate(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'modified_date', *args, **kwargs)

    def test(self):
        self.run_pelican({})

        # Article footer should show the modified date, but it doesn't affect
        # ordering
        self.assertEqual(*self.actual_expected_contents('index.html'))

class Pagination(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'pagination', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'DEFAULT_PAGINATION': 1,
            'PAGINATION_PATTERNS':
                [(1, 'index.html', '{name}.html'),
                 (2, 'index{number}.html', '{name}{number}.html')],
            'DIRECT_TEMPLATES': ['index', 'archives'],
            'PAGINATED_TEMPLATES': {'index': None, 'archives': None, 'tag': None, 'category': None, 'author': None},

            # verify that og:url doesn't take pagination into account
            'M_DISABLE_SOCIAL_META_TAGS': False
        })

        # Every page should contain just one article, only the first page
        # should have it expanded
        self.assertEqual(*self.actual_expected_contents('index.html'))
        self.assertEqual(*self.actual_expected_contents('index2.html'))
        self.assertEqual(*self.actual_expected_contents('index3.html'))

        # Archives are fully equivalent to index (even the links due to patched
        # pagination patterns)
        self.assertEqual(*self.actual_expected_contents('archives.html', 'index.html'))
        self.assertEqual(*self.actual_expected_contents('archives2.html', 'index2.html'))
        self.assertEqual(*self.actual_expected_contents('archives3.html', 'index3.html'))

    def test_categories(self):
        self.run_pelican({
            'DEFAULT_PAGINATION': 1,
            'DIRECT_TEMPLATES': [],

            # verify that og:url doesn't take pagination into account
            'M_DISABLE_SOCIAL_META_TAGS': False
        })

        # Test the category pages as well (same as author/tag). Couldn't test
        # that in the above test case because of the overriden pagination
        # patterns.
        self.assertEqual(*self.actual_expected_contents('category-misc.html'))
        self.assertEqual(*self.actual_expected_contents('category-misc2.html'))
        self.assertEqual(*self.actual_expected_contents('category-misc3.html'))

class PaginationDisabled(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'pagination_disabled', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'DEFAULT_PAGINATION': 1,
            'PAGINATED_TEMPLATES': {'tag': None, 'category': None, 'author': None}
        })

        # Nothing is paginated
        self.assertEqual(*self.actual_expected_contents('index.html'))
        self.assertEqual(*self.actual_expected_contents('archives.html', 'index.html'))

class CollapseFirstGlobal(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'collapse_first_global', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_COLLAPSE_FIRST_ARTICLE': True
        })

        # First article should be collapsed. Index the same as archives.
        self.assertEqual(*self.actual_expected_contents('index.html'))
        self.assertEqual(*self.actual_expected_contents('archives.html', 'index.html'))

class CollapseFirstArticle(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'collapse_first_article', *args, **kwargs)

    def test(self):
        self.run_pelican({})

        # First article should be collapsed. Index the same as archives.
        self.assertEqual(*self.actual_expected_contents('index.html', '../blog_collapse_first_global/index.html'))
        self.assertEqual(*self.actual_expected_contents('archives.html', '../blog_collapse_first_global/index.html'))

class CollapseFirstBoth(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'collapse_first_both', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_COLLAPSE_FIRST_ARTICLE': True
        })

        # Globally enabled, re-enabled in article, so it should be expanded.
        # Index the same as archives.
        self.assertEqual(*self.actual_expected_contents('index.html'))
        self.assertEqual(*self.actual_expected_contents('archives.html', 'index.html'))

class HideSummaryGlobal(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'hide_summary_global', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_HIDE_ARTICLE_SUMMARY': True
        })

        # First (expanded) article summary should be hidden. Index the same as
        # archives.
        self.assertEqual(*self.actual_expected_contents('index.html'))
        self.assertEqual(*self.actual_expected_contents('archives.html', 'index.html'))

        # Both classic and jumbo article should have summary hidden
        self.assertEqual(*self.actual_expected_contents('article.html'))
        self.assertEqual(*self.actual_expected_contents('article-jumbo.html'))

        # Category (and other listing) has always just the summary
        self.assertEqual(*self.actual_expected_contents('category-misc.html'))

class HideSummaryArticle(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'hide_summary_article', *args, **kwargs)

    def test(self):
        self.run_pelican({})

        # First article summary should be shown
        self.assertEqual(*self.actual_expected_contents('index.html', '../blog_hide_summary_global/index.html'))
        self.assertEqual(*self.actual_expected_contents('archives.html', '../blog_hide_summary_global/index.html'))

        # Both classic and jumbo article should have summary hidden
        self.assertEqual(*self.actual_expected_contents('article.html', '../blog_hide_summary_global/article.html'))
        self.assertEqual(*self.actual_expected_contents('article-jumbo.html', '../blog_hide_summary_global/article-jumbo.html'))

        # Category (and other listing) has always just the summary
        self.assertEqual(*self.actual_expected_contents('category-misc.html', '../blog_hide_summary_global/category-misc.html'))

class HideSummaryBoth(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'hide_summary_both', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_HIDE_ARTICLE_SUMMARY': True
        })

        # Globally enabled, re-enabled in article, so it should be shown. Index
        # the same as archives.
        self.assertEqual(*self.actual_expected_contents('index.html'))
        self.assertEqual(*self.actual_expected_contents('index.html'))

        # Both classic and jumbo article should have summary shown
        self.assertEqual(*self.actual_expected_contents('article.html'))
        self.assertEqual(*self.actual_expected_contents('article-jumbo.html'))

        # Category (and other listing) has always just the summary
        self.assertEqual(*self.actual_expected_contents('category-misc.html'))

class CollapseFirstGlobalHideSummaryGlobal(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'collapse_first_global_hide_summary_global', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_COLLAPSE_FIRST_ARTICLE': True,
            'M_HIDE_ARTICLE_SUMMARY': True
        })

        # First article is not expanded, with visible summary. Index the same
        # as archives.
        self.assertEqual(*self.actual_expected_contents('index.html'))
        self.assertEqual(*self.actual_expected_contents('archives.html', 'index.html'))

        # Both classic and jumbo article should have summary hidden
        self.assertEqual(*self.actual_expected_contents('article.html'))
        self.assertEqual(*self.actual_expected_contents('article-jumbo.html'))

class CollapseFirstGlobalHideSummaryArticle(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'collapse_first_global_hide_summary_article', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_COLLAPSE_FIRST_ARTICLE': True
        })

        # First article is not expanded, with visible summary. Index the same
        # as archives.
        self.assertEqual(*self.actual_expected_contents('index.html', '../blog_collapse_first_global_hide_summary_global/index.html'))
        self.assertEqual(*self.actual_expected_contents('archives.html', '../blog_collapse_first_global_hide_summary_global/index.html'))

        # Both classic and jumbo article should have summary hidden
        self.assertEqual(*self.actual_expected_contents('article.html', '../blog_collapse_first_global_hide_summary_global/article.html'))
        self.assertEqual(*self.actual_expected_contents('article-jumbo.html', '../blog_collapse_first_global_hide_summary_global/article-jumbo.html'))

class CollapseFirstGlobalHideSummaryBoth(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'collapse_first_global_hide_summary_both', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_COLLAPSE_FIRST_ARTICLE': True,
            'M_HIDE_ARTICLE_SUMMARY': True
        })

        # First article is not expanded, with visible summary. Index the same
        # as archives.
        self.assertEqual(*self.actual_expected_contents('index.html'))
        self.assertEqual(*self.actual_expected_contents('archives.html', 'index.html'))

        # Both classic and jumbo article should have summary shown
        self.assertEqual(*self.actual_expected_contents('article.html'))
        self.assertEqual(*self.actual_expected_contents('article-jumbo.html'))

class CollapseFirstArticleHideSummaryGlobal(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'collapse_first_article_hide_summary_global', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_HIDE_ARTICLE_SUMMARY': True,
        })

        # First article is not expanded, with visible summary. Index the same
        # as archives.
        self.assertEqual(*self.actual_expected_contents('index.html', '../blog_collapse_first_global_hide_summary_global/index.html'))
        self.assertEqual(*self.actual_expected_contents('archives.html', '../blog_collapse_first_global_hide_summary_global/index.html'))

        # Both classic and jumbo article should have summary hidden
        self.assertEqual(*self.actual_expected_contents('article.html', '../blog_collapse_first_global_hide_summary_global/article.html'))
        self.assertEqual(*self.actual_expected_contents('article-jumbo.html', '../blog_collapse_first_global_hide_summary_global/article-jumbo.html'))

class CollapseFirstArticleHideSummaryArticle(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'collapse_first_article_hide_summary_article', *args, **kwargs)

    def test(self):
        self.run_pelican({})

        # First article is not expanded, with visible summary. Index the same
        # as archives.
        self.assertEqual(*self.actual_expected_contents('index.html', '../blog_collapse_first_global_hide_summary_global/index.html'))
        self.assertEqual(*self.actual_expected_contents('archives.html', '../blog_collapse_first_global_hide_summary_global/index.html'))

        # Both classic and jumbo article should have summary hidden
        self.assertEqual(*self.actual_expected_contents('article.html', '../blog_collapse_first_global_hide_summary_global/article.html'))
        self.assertEqual(*self.actual_expected_contents('article-jumbo.html', '../blog_collapse_first_global_hide_summary_global/article-jumbo.html'))

class CollapseFirstArticleHideSummaryBoth(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'collapse_first_article_hide_summary_both', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_HIDE_ARTICLE_SUMMARY': True
        })

        # First article is not expanded, with visible summary. Index the same
        # as archives.
        self.assertEqual(*self.actual_expected_contents('index.html', '../blog_collapse_first_global_hide_summary_both/index.html'))
        self.assertEqual(*self.actual_expected_contents('archives.html', '../blog_collapse_first_global_hide_summary_both/index.html'))

        # Both classic and jumbo article should have summary shown
        self.assertEqual(*self.actual_expected_contents('article.html', '../blog_collapse_first_global_hide_summary_both/article.html'))
        self.assertEqual(*self.actual_expected_contents('article-jumbo.html', '../blog_collapse_first_global_hide_summary_both/article-jumbo.html'))

class CollapseFirstBothHideSummaryGlobal(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'collapse_first_both_hide_summary_global', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_COLLAPSE_FIRST_ARTICLE': True,
            'M_HIDE_ARTICLE_SUMMARY': True
        })

        # First (expanded) article summary should be shown. Index the same as
        # archives.
        self.assertEqual(*self.actual_expected_contents('index.html'))
        self.assertEqual(*self.actual_expected_contents('archives.html', 'index.html'))

        # Both classic and jumbo article should have summary hidden
        self.assertEqual(*self.actual_expected_contents('article.html'))
        self.assertEqual(*self.actual_expected_contents('article-jumbo.html'))

class CollapseFirstBothHideSummaryArticle(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'collapse_first_both_hide_summary_article', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_COLLAPSE_FIRST_ARTICLE': True
        })

        # First (expanded) article summary should be shown. Index the same as
        # archives.
        self.assertEqual(*self.actual_expected_contents('index.html', '../blog_collapse_first_both_hide_summary_global/index.html'))
        self.assertEqual(*self.actual_expected_contents('archives.html', '../blog_collapse_first_both_hide_summary_global/index.html'))

        # Both classic and jumbo article should have summary hidden
        self.assertEqual(*self.actual_expected_contents('article.html', '../blog_collapse_first_both_hide_summary_global/article.html'))
        self.assertEqual(*self.actual_expected_contents('article-jumbo.html', '../blog_collapse_first_both_hide_summary_global/article-jumbo.html'))

class CollapseFirstBothHideSummaryBoth(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'collapse_first_both_hide_summary_both', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_COLLAPSE_FIRST_ARTICLE': True,
            'M_HIDE_ARTICLE_SUMMARY': True
        })

        # First (expanded) article summary should be shown. Index the same as
        # archives.
        self.assertEqual(*self.actual_expected_contents('index.html'))
        self.assertEqual(*self.actual_expected_contents('archives.html', 'index.html'))

        # Both classic and jumbo article should have summary shown
        self.assertEqual(*self.actual_expected_contents('article.html'))
        self.assertEqual(*self.actual_expected_contents('article-jumbo.html'))

class HtmlEscape(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'html_escape', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'SITENAME': '<&> in site name',
            'M_BLOG_NAME': '<&> in blog name',
            'M_BLOG_URL': 'archives.html?and&in&url=""',
            'M_BLOG_FAVICON': ('favicon.ico?and&in&url=""', 'huh&what'),
            'ARTICLE_URL': '{slug}.html?and&in&url=""',
            'AUTHOR_URL': 'author-{slug}.html?and&in&url=""',
            'CATEGORY_URL': 'category-{slug}.html?and&in&url=""',
            'TAG_URL': 'tag-{slug}.html?and&in&url=""',
            'M_LINKS_FOOTER2': [('An <&> in link', '#')],
            'M_SHOW_AUTHOR_LIST': True,
            'M_DISABLE_SOCIAL_META_TAGS': False, # to verify escaping in these
            'DEFAULT_PAGINATION': 1,
            'PAGINATION_PATTERNS':
                [(1, 'index.html?and&in&url=""', '{name}.html'),
                 (2, 'index{number}.html?and&in&url=""', '{name}{number}.html')],
            'DIRECT_TEMPLATES': ['index', 'archives'],
            'PAGINATED_TEMPLATES': {'index': None, 'archives': None, 'tag': None, 'category': None, 'author': None},
            'FORMATTED_FIELDS': ['summary', 'description']
        })

        # Verify that everything is properly escaped everywhere
        self.assertEqual(*self.actual_expected_contents('index.html'))
        self.assertEqual(*self.actual_expected_contents('index2.html'))

        # Archives are almost the same as index, except for og:url pointing to
        # M_BLOG_URL instead of SITEURL
        self.assertEqual(*self.actual_expected_contents('archives.html'))
        self.assertEqual(*self.actual_expected_contents('archives2.html'))

        self.assertEqual(*self.actual_expected_contents('article.html'))
        self.assertEqual(*self.actual_expected_contents('article-jumbo.html'))

class GlobalSocialMeta(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'global_social_meta', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_BLOG_DESCRIPTION': 'This is not displayed anywhere.',
            'M_SOCIAL_TWITTER_SITE': '@czmosra',
            'M_SOCIAL_TWITTER_SITE_ID': '1537427036',
            'M_SOCIAL_IMAGE': 'http://your.brand/static/site.png',
            'M_SOCIAL_BLOG_SUMMARY': 'This is also not displayed anywhere.',
            'M_DISABLE_SOCIAL_META_TAGS': False
        })

        # Verify that the social meta tags are present in all pages
        self.assertEqual(*self.actual_expected_contents('article.html'))
        self.assertEqual(*self.actual_expected_contents('category-a-category.html'))
        self.assertEqual(*self.actual_expected_contents('author-the-author.html'))
        self.assertEqual(*self.actual_expected_contents('tag-a-tag.html'))

class ArchivedArticle(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'archived_article', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_ARCHIVED_ARTICLE_BADGE': """
.. container:: m-note m-warning

    This article is from {year}. **It's old.** Deal with it.
"""
        })

        self.assertEqual(*self.actual_expected_contents('article.html'))
        self.assertEqual(*self.actual_expected_contents('article-jumbo.html'))

class GlobalFavicon(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'global_favicon', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_FAVICON': ('favicon.png', 'image/png'),
            'M_DISABLE_SOCIAL_META_TAGS': True
        })

        self.assertEqual(*self.actual_expected_contents('index.html'))

class NewsOnIndex(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'news_on_index', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_BLOG_URL': 'http://our.blog/',
            'M_NEWS_ON_INDEX': ("Latest rants on our blog", 2)
        })

        self.assertEqual(*self.actual_expected_contents('index.html'))

class Draft(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'draft', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'DRAFT_URL': '{slug}.html',
            'DRAFT_SAVE_AS': '{slug}.html'
        })

        self.assertEqual(*self.actual_expected_contents('article.html'))
        self.assertEqual(*self.actual_expected_contents('article-jumbo.html'))

class Feeds(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'feeds', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_DISABLE_SOCIAL_META_TAGS': True,
            'FEED_ALL_ATOM': 'atom.xml',
            'FEED_ALL_ATOM_URL': 'feeds/atom.xml',
            'CATEGORY_FEED_ATOM': '{slug}.atom.xml',
            'CATEGORY_FEED_ATOM_URL': 'feeds/{slug}.atom.xml',
        })

        # There should be just the first column
        self.assertEqual(*self.actual_expected_contents('index.html'))
        self.assertEqual(*self.actual_expected_contents('category-a-category.html'))

class FeedsNoUrl(BlogTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'feeds_no_url', *args, **kwargs)

    def test(self):
        self.run_pelican({
            'M_DISABLE_SOCIAL_META_TAGS': True,
            'FEED_ALL_ATOM': 'feeds/atom.xml',
            'CATEGORY_FEED_ATOM': 'feeds/{slug}.atom.xml',
        })

        # The feed URLs should be the same as above
        self.assertEqual(*self.actual_expected_contents('index.html', '../blog_feeds/index.html'))
        self.assertEqual(*self.actual_expected_contents('category-a-category.html', '../blog_feeds/category-a-category.html'))
