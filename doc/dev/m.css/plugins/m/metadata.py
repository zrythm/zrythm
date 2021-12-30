#
#   This file is part of m.css.
#
#   Copyright © 2017, 2018, 2019, 2020 Vladimír Vondruš <mosra@centrum.cz>
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

import logging
import os

from pelican import signals

logger = logging.getLogger(__name__)

_names = {'authors': "Author",
          'categories': "Category",
          'tags': "Tag"}

def _read_pages(article_generator, key):
    path = article_generator.settings.get(*key)
    fullpath = os.path.join(article_generator.settings['PATH'], path)
    if not os.path.isdir(fullpath): return {}

    pages = {}
    for f in os.listdir(fullpath):
        fullf = os.path.join(fullpath, f)
        if not os.path.isfile(fullf): continue

        logger.debug("Read file {} -> {}".format(os.path.join(path, f), {
            'authors': "Author",
            'categories': "Category",
            'tags': "Tag"}[key[1]]))

        base_path, filename = os.path.split(fullf)
        pages[os.path.splitext(filename)[0]] = article_generator.readers.read_file(base_path, filename, context=article_generator.context)
    return pages

def populate_metadata(article_generator):
    authors = _read_pages(article_generator, ('M_METADATA_AUTHOR_PATH', 'authors'))
    categories = _read_pages(article_generator, ('M_METADATA_CATEGORY_PATH', 'categories'))
    tags = _read_pages(article_generator, ('M_METADATA_TAG_PATH', 'tags'))

    for author, _ in article_generator.authors:
        author.page = authors.get(author.slug, {})
    for category, _ in article_generator.categories:
        category.page = categories.get(category.slug, {})
    for tag in article_generator.tags:
        tag.page = tags.get(tag.slug, {})

    for article in article_generator.articles:
        page = authors.get(article.author.slug, {})
        for i in ['badge', 'image', 'twitter', 'twitter_id']:
            if hasattr(page, i): setattr(article.author, i, getattr(page, i))
        if hasattr(page, 'title'):
            article.author.badge_title = page.title

        page = categories.get(article.category.slug, {})
        for i in ['badge', 'image']:
            if hasattr(page, i): setattr(article.category, i, getattr(page, i))
        if hasattr(page, 'title'):
            article.category.badge_title = page.title

def register_mcss(**kwargs):
    assert not "This plugin is Pelican-only" # pragma: no cover

def register(): # for Pelican
    signals.article_generator_finalized.connect(populate_metadata)
