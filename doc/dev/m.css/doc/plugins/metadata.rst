..
    This file is part of m.css.

    Copyright © 2017, 2018, 2019, 2020 Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
..

Metadata
########

:breadcrumb: {filename}/plugins.rst Plugins
:summary: Assigns additional description and images to categories, authors and
    tags in Pelican-powered sites.
:footer:
    .. note-dim::
        :class: m-text-center

        `« Links and other <{filename}/plugins/links.rst>`_ | `Plugins <{filename}/plugins.rst>`_ | `Sphinx » <{filename}/plugins/sphinx.rst>`_

.. role:: html(code)
    :language: html
.. role:: py(code)
    :language: py
.. role:: rst(code)
    :language: rst

Assigns additional description and images to categories, authors and tags in
Pelican-powered sites. As it extends a Pelican-specific feature, it has no
equivalent for other themes.

.. contents::
    :class: m-block m-default

`How to use`_
=============

Download the `m/metadata.py <{filename}/plugins.rst>`_ file, put it including
the ``m/`` directory into one of your :py:`PLUGIN_PATHS` and add ``m.metadata``
package to your :py:`PLUGINS` in ``pelicanconf.py``. In order to have the
contents properly rendered, it's advised to add the new fields to
:py:`FORMATTED_FIELDS`:

.. code:: py

    PLUGINS += ['m.metadata']
    FORMATTED_FIELDS += ['description', 'badge']

This plugin parses additional info from pages in subdirectories of the contents
directory (i.e., where the :py:`PATH` setting points to). By default, the
directories are as follows and can be configured with these settings:

.. code:: py

    M_METADATA_AUTHOR_PATH = 'authors'
    M_METADATA_CATEGORY_PATH = 'categories'
    M_METADATA_TAG_PATH = 'tags'

The m.css Pelican theme recognizes presence of this plugin and renders the
additional metadata both in the page and in the `Open Graph <http://ogp.me/>`_
and `Twitter Card <https://developer.twitter.com/en/docs/tweets/optimize-with-cards/overview/summary-card-with-large-image>`_
social :html:`<meta>` tags, in addition to tags rendered by
`articles themselves <{filename}/themes/pelican.rst#social-meta-tags-for-articles>`_.
See below for more information.

`Author metadata`_
==================

Author pages are matched to authors based on the slug (so e.g. metadata for
author named *John Doe* will be in a file named ``authors/john-doe.rst``). The
following metadata are recognized:

-   Page content is exposed to the theme in :py:`author.page.content` on the
    author page and used to fill author details block on top of the author page
    in the m.css Pelican theme. If not set, no author details block is
    rendered.
-   Page title is exposed to the theme in :py:`author.page.title` on the author
    page and in :py:`article.author.badge_title` in all articles having given
    author. In the m.css Pelican theme it is used as caption in author details
    block and in ``og:title`` / ``twitter:title`` social :html:`<meta>` tag on
    author page. Can be used to provide a longer version of author name on this
    page. If not set, the m.css Pelican theme uses the global author name (the
    one set by articles) instead --- but note that since 3.8, Pelican will warn
    about this and suggest to specify the title explicitly.
-   The :rst:`:description:` field is exposed to the theme in
    :py:`author.page.description` on the author page. The m.css Pelican theme
    uses it in :html:`<meta name="description">`. If not set, no tag is
    rendered.
-   The :rst:`:summary:` field is exposed to the theme in :py:`author.page.summary`
    on the author page. The m.css Pelican theme uses it in ``og:description``
    / ``twitter:description`` social :html:`<meta>` tag. If not set, no tag is
    rendered.
-   The :rst:`:badge:` field is exposed to the theme in :py:`article.author.badge`
    in all articles having given author. The m.css Pelican theme uses it to
    display an *About the author* badge at the end of article pages. Similarly
    to page content, it's possible to use advanced :abbr:`reST <reStructuredText>`
    formatting capabilities here. If not set, no badge at the end of author's
    articles is rendered.
-   The :rst:`:image:` field is exposed to the theme in :py:`author.page.image`
    on the author page and in :py:`article.author.image` in all articles having
    given author. The m.css Pelican theme uses it to add an image to author
    badge on articles, to author details on author page and in ``og:image`` /
    ``twitter:image`` social :html:`<meta>` tags on author page, overriding the
    global :py:`M_SOCIAL_IMAGE`. It's expected to be smaller and square
    similarly to the :py:`M_SOCIAL_IMAGE` `described in the theme documentation <{filename}/themes/pelican.rst#global-site-image>`_.
    If not set, the details and badges are rendered without images and no
    social tag is present. Does not affect ``twitter:card``, it's set to
    ``summary`` regardless of whether the image is present or not.
-   The :rst:`:twitter:` / :rst:`:twitter_id:` fields are exposed to the theme
    in :py:`article.author.twitter` / :py:`article.author.twitter_id` and
    :py:`author.page.twitter` / :py:`author.page.twitter_id`. The m.css
    Pelican theme uses it to render ``twitter:creator`` / ``twitter::creator:id``
    social :html:`<meta>` tags. If not set, no tags are rendered.

Example of a completely filled author page, saved under ``authors/john-doe.rst``
and matching an author named *John Doe*:

.. code:: rst

    John "Not That Serial Killer" Doe
    #################################

    :twitter: @se7en
    :twitter_id: 7777777
    :image: {static}/authors/john-doe.jpg
    :description: I'm not that serial killer.
    :summary: I'm really not that serial killer.
    :badge: No, really, don't confuse me with that guy.

    What? No, I didn't kill anybody. Yet.

.. note-info::

    See how author info is rendered in the m.css Pelican theme
    `on the author page <{author}an-author>`_ and
    `on the article page <{filename}/examples/article.rst>`_.

`Category metadata`_
====================

Category pages are matched to categories based on the slug (so e.g. metadata
for category named *Guest posts* will be in a file named
``categories/guest-posts.rst``). The following metadata are recognized:

-   Page content is exposed to the theme in :py:`category.page.content` on the
    category page and used to fill category details block on top of the
    category page in the m.css Pelican theme. If not set, no category details
    block is rendered.
-   Page title is exposed to the theme in :py:`category.page.title` on the
    category page and in :py:`article.category.badge_title` in all articles
    being in given category. In the m.css Pelican theme it is used as caption
    in category details block, in ``og:title`` / ``twitter:title`` social
    :html:`<meta>` tag on category page and as badge title on article pages.
    Can be used to provide a longer version of category name for article badge
    and detailed category info. If not set, the m.css Pelican theme uses the
    global category name (the one set by articles) instead --- but note that
    since 3.8, Pelican will warn about this and suggest to specify the title
    explicitly.
-   The :rst:`:description:` field is exposed to the theme in
    :py:`category.page.description` on the category page. The m.css Pelican
    theme uses it in :html:`<meta name="description">`. If not set, no tag is
    rendered.
-   The :rst:`:summary:` field is exposed to the theme in :py:`category.page.summary`
    on the category page. The m.css Pelican theme uses it in ``og:description``
    / ``twitter:description`` social :html:`<meta>` tag. If not set, no tag is
    rendered.
-   The :rst:`:badge:` field is exposed to the theme in :py:`article.category.badge`
    in all articles being in given category. The m.css Pelican theme uses it to
    display an informational badge at the end of article pages. Similarly to
    page content, it's possible to use advanced :abbr:`reST <reStructuredText>`
    formatting capabilities here. If not set, no badge at the end of articles
    in given category is rendered.
-   The :rst:`:image:` field is exposed to the theme in :py:`category.page.image`
    on the category page and in :py:`article.category.image` in all articles
    being in given category. The m.css Pelican theme uses it to add an image to
    category badges on articles, to category details on category page and
    in ``og:image`` / ``twitter:image`` social :html:`<meta>` tags on category
    page. If `article cover image <{filename}/themes/pelican.rst#jumbo-articles>`_
    is not specified, the image is used also for ``og:image`` / ``twitter:image``
    on given article, overriding the global :py:`M_SOCIAL_IMAGE`. It's expected
    to be smaller and square similarly to the
    :py:`M_SOCIAL_IMAGE` `described in the theme documentation <{filename}/themes/pelican.rst#global-site-image>`_.
    If not set, the details and badges are rendered without images and no
    social tag is present. Does not affect ``twitter:card``, it's set to
    ``summary`` or ``summary_large_image`` depending only on presence of
    article cover image.

Example of a completely filled category page, saved under ``categories/guest-posts.rst``
and matching a category named *Guest posts*:

.. code:: rst

    Posts by our users
    ##################

    :image: {static}/categories/guest-posts.jpg
    :description: User stories and product reviews
    :summary: Stories of our users and honest reviews of our product.
    :badge: This article is a guest post. Want to share your story as well? Head
        over to the `intro article <{filename}/blog/introducing-guest-posts.rst>`_
        to get to know more. We'll happily publish it here.

    This section contains guest posts, reviews and success stories. Want to share
    your story as well? Head over to the
    `intro article <{filename}/blog/introducing-guest-posts.rst>`_ to get to know
    more. We'll happily publish it here.

.. note-info::

    See how category info is rendered in the m.css Pelican theme
    `on the category page <{category}examples>`_ and
    `on the article page <{filename}/examples/article.rst>`_.

`Tag metadata`_
===============

Tag pages are matched to authors based on the slug (so e.g. metadata for
tag named *Pantomime* will be in a file named ``tags/pantomime.rst``). The
following metadata are recognized:

-   Page content is exposed to the theme in :py:`tag.page.content` on the tag
    page and used to fill tag details block on top of the tag page in the m.css
    Pelican theme. If not set, no tag details block is rendered.
-   Page title is exposed to the theme in :py:`tag.page.title` on the tag page,
    is used as caption in tag details block on tag page and in ``og:title`` /
    ``twitter:title`` social :html:`<meta>` tag. Can be used to provide a
    longer version of tag name on this page. If not set, the m.css Pelican
    theme uses the global tag name (the one set by articles) instead --- but
    note that since 3.8, Pelican will warn about this and suggest to specify
    the title explicitly.
-   The :rst:`:description:` field is exposed to the theme in
    :py:`tag.page.description` on the tag page. The m.css Pelican theme uses
    it in :html:`<meta name="description">`. If not set, no :html:`<meta>` tag
    is rendered.
-   The :rst:`:summary:` field is exposed to the theme in :py:`tag.page.summary`
    on the tag page. The m.css Pelican theme uses it in ``og:description``
    / ``twitter:description`` social :html:`<meta>` tag. If not set, no
    :html:`<meta>` tag is rendered.

Example of a completely filled tag page, saved under ``tags/pantomime.rst``
and matching a tag named *Pantomime*:

.. code:: rst

    ¯\_(ツ)_/¯
    ##########

    :description: ¯\_(ツ)_/¯
    :summary: ¯\_(ツ)_/¯

    ¯\_(ツ)_/¯

.. note-info::

    See how tag info is rendered `in the m.css Pelican theme <{tag}Jumbo>`_.
