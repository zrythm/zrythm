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

Pelican
#######

:alias:
    /pelican/index.html
    /pelican/theme/index.html
:breadcrumb: {filename}/themes.rst Themes
:footer:
    .. note-dim::
        :class: m-text-center

        `« Writing reST content <{filename}/themes/writing-rst-content.rst>`_ | `Themes <{filename}/themes.rst>`_

.. role:: html(code)
    :language: html
.. role:: py(code)
    :language: py
.. role:: rst(code)
    :language: rst
.. role:: sh(code)
    :language: sh
.. |x| unicode:: U+00D7 .. nicer multiply sign

`Pelican <https://getpelican.com/>`_ is a static site generator powered by
Python and unlike most other static site generators, it uses
`reStructuredText <http://docutils.sourceforge.net/rst.html>`_ instead of
Markdown for authoring content. m.css provides a theme for it, together with a
set of useful plugins. The theme is designed to fit both the use case of a
simple blog consisting of just articles or a full product/project/portfolio
website where the blog is only a side dish.

.. button-success:: https://magnum.graphics

    Live demo

    magnum.graphics

.. contents::
    :class: m-block m-default

`First steps with Pelican`_
===========================

Install Pelican either via ``pip`` or using your system package manager.

.. note-danger::

    In order to use the m.css theme or `plugins <{filename}/plugins.rst>`_, you
    need to install Pelican 4 and the Python 3 version of it. The theme and
    plugins work with Python 3.5 and newer. Python 2 is not supported and
    compatibility with Pelican 3.7 has been dropped.

.. code:: sh

    # You may need sudo here
    pip3 install pelican

Note that using the m.css theme or Pelican plugins may require additional
dependencies than just Pelican --- documentation of each lists the additional
requirements, if any. Once you have Pelican installed, create a directory for
your website and bootstrap it:

.. code:: sh

    mkdir ~/my-cool-site/
    cd ~/my-cool-site/
    pelican-quickstart

This command will ask you a few questions. Leave most of the questions at their
defaults, you can change them later. Once the quickstart script finishes, you
can generate the website and spin up a auto-reload webserver for it with the
following command:

.. code:: sh

    pelican -Dlr

It will print quite some output about processing things and serving the data to
the console. Open your fresh website at http://localhost:8000. The site is now
empty, so let's create a simple article and put it into ``content/``
subdirectory with a ``.rst`` extension. For this example that would be
``~/my-cool-site/content/my-cool-article.rst``:

.. code:: rst

    My cool article
    ###############

    :date: 2017-09-14 23:04
    :category: Cool things
    :tags: cool, article, mine
    :summary: This article has a cool summary.

    This article has not only cool summary, but also has cool contents. Lorem?
    *Ipsum.* `Hi, google! <https://google.com>`_

If you did everything right, the auto-reload script should pick the file up and
process it (check the console output). Then it's just a matter of refreshing
your browser to see it on the page.

*That's it!* Congratulations, you successfully made your first steps with
Pelican. Well, apart from the fact that the default theme is a bit
underwhelming --- so let's fix that.

`Basic theme setup`_
====================

The easiest way to start is putting the
:gh:`whole Git repository <mosra/m.css>` of m.css into your project, for
example as a submodule:

.. code:: sh

    git submodule add git://github.com/mosra/m.css

The most minimal configuration to use the theme is the following. Basically you
need to tell Pelican where the theme resides (it's in the ``pelican-theme/``
subdir of your m.css submodule), then you tell it to put the static contents of
the theme into a ``static/`` directory in the root of your webserver; the
:py:`M_CSS_FILES` variable is a list of CSS files that the theme needs. You can
put there any files you need, but there need to be at least the files mentioned
on the `CSS themes <{filename}/css/themes.rst>`_ page. The :py:`M_THEME_COLOR`
specifies color used for the :html:`<meta name="theme-color" />` tag
corresponding to given theme; if not set, it's simply not present. Lastly, the
theme uses some Jinja2 filters from the `m.htmlsanity <{filename}/plugins/htmlsanity.rst>`_
plugin, so that plugin needs to be loaded as well.

.. code:: py

    THEME = 'm.css/pelican-theme'
    THEME_STATIC_DIR = 'static'
    DIRECT_TEMPLATES = ['index']

    M_CSS_FILES = ['https://fonts.googleapis.com/css?family=Source+Sans+Pro:400,400i,600,600i%7CSource+Code+Pro:400,400i,600',
                   '/static/m-dark.css']
    M_THEME_COLOR = '#22272e'

    PLUGIN_PATHS = ['m.css/plugins']
    PLUGINS = ['m.htmlsanity']

Here you can take advantage of the ``pelicanconf.py`` and ``publishconf.py``
distinction --- use ``m-dark.css`` for local development and override the
:py:`M_CSS_FILES` to use the smaller, faster and more compatible ``m-dark.compiled.css``
for publishing.

If you would want to use the light theme instead, the configuration is this
(again with ``m-light.css`` possibly replaced with ``m-light.compiled.css``):

.. code:: py

    M_CSS_FILES = ['https://fonts.googleapis.com/css?family=Libre+Baskerville:400,400i,700,700i%7CSource+Code+Pro:400,400i,600',
                   '/static/m-light.css']
    M_THEME_COLOR = '#cb4b16'

.. note-info::

    To reduce confusion, new configuration variables specific to m.css theme
    and plugins are prefixed with ``M_``. Configuration variables without
    prefix are builtin Pelican options.

If you see something unexpected or not see something expected, check the
`Troubleshooting`_ section below.

`Configuration`_
================

Value of :py:`SITENAME` is used in the :html:`<title>` tag, separated with a
``|`` character from page title. If page title is the same as :py:`SITENAME`
(for example on the index page), only the page title is shown. The static part
of the website with pages is treated differently from the "blog" part with
articles and there are two additional configuration options :py:`M_BLOG_URL` and
:py:`M_BLOG_NAME` that control how various parts of the theme link to the blog
and how blog pages are named in the :html:`<title>` element. The :py:`M_BLOG_URL`
can be either absolute or relative to :py:`SITEURL`. If :py:`M_BLOG_NAME` /
:py:`M_BLOG_URL` are not set, the theme assumes they are the same as
:py:`SITENAME` / :py:`SITEURL`.

.. code:: py

    SITENAME = 'Your Brand'
    SITEURL = ''

    M_BLOG_NAME = 'Your Brand Blog'
    M_BLOG_URL = 'blog/'

The :py:`M_FAVICON` setting, if present, is used to specify contents of the
:html:`<link rel="icon">` tag. It's a tuple of :py:`(url, type)` where
:py:`url` is favicon URL and :py:`type` is its corresponding MIME type. If
:py:`M_BLOG_FAVICON` is specified, it's overriding :py:`M_FAVICON` on blog-like
pages (articles, article listing... basically everything except pages). If
:py:`M_BLOG_FAVICON` is not specified, :py:`M_FAVICON` is used everywhere; if
neither is specified no :html:`<link>` tag is rendered. Example configuration:

.. code:: py

    M_FAVICON = ('favicon.ico', 'image/x-ico')
    M_BLOG_FAVICON = ('favicon-blog.png', 'image/png')

Arbitrary HTML content can be added at the end of the :html:`<head>` via the
:py:`M_HTML_HEADER` option.

`Top navbar`_
-------------

:py:`M_SITE_LOGO` is an image file that will be used as a brand logo on left
side of the navbar, :py:`M_SITE_LOGO_TEXT` is brand logo text. Specifying just
one of these does the expected thing, if neither of them is specified, the
theme will use :py:`SITENAME` in place of :py:`M_SITE_LOGO_TEXT`. The brand
logo/text is a link that leads to :py:`SITTEURL`.

:py:`M_LINKS_NAVBAR1` and :py:`M_LINKS_NAVBAR2` variables contain links to put
in the top navbar. On narrow screens, the navbar is divided into two columns,
links from the first variable are in the left column while links from the
second variable are in the right column. Omit the second variable if you want
the links to be in a single column. Omitting both variables will cause the
hamburger menu link on small screen sizes to not even be present.

Both variables have the same format --- a list of 4-tuples, where first item is
link title, second the URL, third page slug of the corresponding page (used
to highlight currently active menu item) and fourth is a list of sub-menu items
(which are 3-tuples --- link title, URL and page slug). Providing an empty slug
will make the menu item never highlighted; providing an empty list of sub-menu
items will not add any submenu. All blog-related pages (articles, article
listing, authors, tags, categories etc.) have the slug set to a special value
``[blog]``. The URL is prepended with :py:`SITEURL` unless it contains also
domain name, then it's left as-is (`detailed behavior <{filename}/plugins/htmlsanity.rst#siteurl-formatting>`_).

Example configuration, matching example markup from the
`CSS page layout <{filename}/css/page-layout.rst#sub-menus-in-the-navbar>`__
documentation:

.. code:: py

    M_SITE_LOGO_TEXT = 'Your Brand'

    M_LINKS_NAVBAR1 = [('Features', 'features/', 'features', []),
                       ('Showcase', 'showcase/', 'showcase', []),
                       ('Download', 'download/', 'download', [])]

    M_LINKS_NAVBAR2 = [('Blog', 'blog/', '[blog]', [
                            ('News', 'blog/news/', ''),
                            ('Archive', 'blog/archive/', '')]),
                       ('Contact', 'contact/', 'contact', [])]

`Footer navigation`_
--------------------

Similarly to the top navbar, :py:`M_LINKS_FOOTER1`, :py:`M_LINKS_FOOTER2`,
:py:`M_LINKS_FOOTER3` and :py:`M_LINKS_FOOTER4` variables contain links to put
in the footer navigation. The links are arranged in four columns, which get
reduced to just two columns on small screens. Omitting :py:`M_LINKS_FOOTER4`
will fill the last column with a *Blog* entry, linking to the Archives page and
listing all blog categories; you can disable that entry by setting
:py:`M_LINKS_FOOTER4 = []`. Omitting any of the remaining variables will make
given column empty, omitting all variables will not render the navigation at
all.

The variables are lists of 2-tuples, containing link title and URL. First item
is used for column header, if link URL of the first item is empty, given column
header is just a plain :html:`<h3>` without a link. The URLs are processed in
the same way as in the `top navbar`_. A tuple entry with empty title (i.e.,
:py:`('', '')`) will put a spacer into the list.

Footer fine print can be specified via :py:`M_FINE_PRINT`. Contents of the
variable are processed as :abbr:`reST <reStructuredText>`, so you can use all
the formatting and linking capabilities in there. If :py:`M_FINE_PRINT` is not
specified, the theme will use the following instead. Set
:py:`M_FINE_PRINT = None` to disable rendering of the fine print completely.

.. code:: py

    M_FINE_PRINT = SITENAME + """. Powered by `Pelican <https://getpelican.com>`_
        and `m.css <https://mcss.mosra.cz>`_."""

If :py:`M_FINE_PRINT` is set to :py:`None` and none of :py:`M_LINKS_FOOTER1`,
:py:`M_LINKS_FOOTER2`, :py:`M_LINKS_FOOTER3`, :py:`M_LINKS_FOOTER4` is set, the
footer is not rendered at all.

Example configuration, again matching example markup from the
`CSS page layout <{filename}/css/page-layout.rst#footer-navigation>`__
documentation, populating the last column implicitly:

.. code:: py

    M_LINKS_FOOTER1 = [('Your Brand', '/'),
                       ('Features', 'features/'),
                       ('Showcase', 'showcase/')]

    M_LINKS_FOOTER2 = [('Download', 'download/'),
                       ('Packages', 'download/packages/'),
                       ('Source', 'download/source/')]

    M_LINKS_FOOTER3 = [('Contact', ''),
                       ('E-mail', 'mailto:you@your.brand'),
                       ('GitHub', 'https://github.com/your-brand')]

    M_FINE_PRINT = """
    Your Brand. Copyright © `You <mailto:you@your.brand>`_, 2017. All rights
    reserved.
    """

`(Social) meta tags`_
---------------------

The :rst:`M_BLOG_DESCRIPTION` setting, if available, is used to populate
:html:`<meta name="description">` on the index / archive page, which can be
then shown in search engine results. For sharing pages on Twitter, Facebook and
elsewhere, it's possible to configure site-wide `Open Graph <http://ogp.me/>`_
and `Twitter Card <https://developer.twitter.com/en/docs/tweets/optimize-with-cards/overview/summary-card-with-large-image>`_
:html:`<meta>` tags:

-   ``og:site_name`` is set to :py:`M_SOCIAL_SITE_NAME`, if available
-   ``twitter:site`` / ``twitter:site:id`` is set to :py:`M_SOCIAL_TWITTER_SITE`
    / :py:`M_SOCIAL_TWITTER_SITE_ID``, if available
-   Global ``og:title`` / ``twitter:title`` is set to :py:`M_BLOG_NAME` on
    index and archive pages and to category/author/tag name on particular
    filtering pages. This is overridden by particular pages and articles.
-   Global ``og:url`` is set to :py:`M_BLOG_URL` on index and archive pages and
    to category/author/tag URL on particular filtering pages. Pagination is
    *not* included in the URL. This is overridden by particular pages and
    articles.
-   Global ``og:image`` / ``twitter:image`` is set to the
    :py:`M_SOCIAL_IMAGE` setting, if available. The image is expected to be
    smaller and square; Pelican internal linking capabilities are *not*
    supported in this setting. This can be overridden by particular pages and
    articles.
-   Global ``twitter:card`` is set to ``summary``. This is further affected by
    metadata of particular pages and articles.
-   Global ``og:description`` / ``twitter:description`` is set to
    :py:`M_SOCIAL_BLOG_SUMMARY` on index and archive pages.
-   Global ``og:type`` is set to ``website``. This is overridden by particular
    pages and articles.

See `(Social) meta tags for pages`_ and `(Social) meta tags for articles`_
sections below for page- and article-specific :html:`<meta>` tags.

.. note-danger::

    The :html:`<meta name="keywords">` tag is not supported, as it doesn't
    have any effect on search engine results at all.

Example configuration to give sane defaults to all social meta tags:

.. code:: py

    M_BLOG_NAME = "Your Brand Blog"
    M_BLOG_URL = 'https://blog.your.brand/'
    M_BLOG_DESCRIPTION = "Your Brand is the brand that provides all that\'s needed."

    M_SOCIAL_TWITTER_SITE = '@your.brand'
    M_SOCIAL_TWITTER_SITE_ID = 1234567890
    M_SOCIAL_IMAGE = 'https://your.brand/static/site.png'
    M_SOCIAL_BLOG_SUMMARY = "This is the brand you need"

.. _global-site-image:

.. block-success:: Recommended sizes for global site image

    The theme assumes that the global site image is smaller and square in order
    to appear just as a small thumbnail next to a link, not as large cover
    image above it --- the reasoning behind is that there's no point in annoying
    the users by decorating the global site links with the exact same large
    image.

    For Twitter, this is controlled explicitly by setting ``twitter:card``
    to ``summary`` instead of ``summary_large_image``, but in case of Facebook,
    it's needed to rely on their autodetection.
    `Their documentation <https://developers.facebook.com/docs/sharing/best-practices/#images>`_
    says that images smaller than 600\ |x|\ 315 px are displayed as small
    thumbnails. Square image of size 256\ |x|\ 256 px is known to work well.

    Note that the assumptions are different for pages and articles with
    explicit cover images, see `(Social) meta tags for pages`_ below for
    details.

.. note-info::

    You can see how links for default pages will display by pasting
    URL of the `article listing page <{category}examples>`_ into either
    `Facebook Debugger <https://developers.facebook.com/tools/debug/>`_ or
    `Twitter Card Validator <https://cards-dev.twitter.com/validator>`_.

It's possible to disable rendering of all social meta tags (for example for
testing purposes) by setting :py:`M_DISABLE_SOCIAL_META_TAGS` to :py:`True`.

`Pages`_
========

Page content is simply put into :html:`<main>`, wrapped in an :html:`<article>`,
in the center 10 columns on large screens and spanning the full 12 columns
elsewhere; the container is marked as `inflatable <{filename}/css/grid.rst#inflatable-nested-grid>`_.
Page title is rendered in an :html:`<h1>` and there's nothing else apart from
the page content.

Pages can override which menu item in the `top navbar`_ will be highlighted
by specifying the corresponding menu item slug in the :rst:`:highlight:` field.
If the field is not present, page's own slug is used instead.

`Extending HTML \<head\>`_
--------------------------

The :rst:`:css:` field can be used to link additional CSS files in page header.
Put one URL per line, internal link targets are expanded. Similarly :rst:`:js:`
can be used to link JavaScript files. Besides that, the :rst:`:html_header:`
field can be used to put arbitrary HTML at the end of the :html:`<head>`
element. Indenting the lines is possible by putting an escaped space in front
(the backslash and the escaped space itself won't get inserted). Example:

.. code:: rst

    Showcase
    ########

    :css:
        {static}/static/webgl.css
        {static}/static/canvas-controls.css
    :js:
        {static}/static/showcase.js
    :html_header:
        <script>
        \   function handleDrop(event) {
        \     event.preventDefault();
        \     ...
        \   }
        </script>

`Breadcrumb navigation`_
------------------------

It's common for pages to be organized in a hierarchy and the user should be
aware of it. m.css Pelican theme provides breadcrumb navigation, which is
rendered in main page heading (as described in the
`CSS page layout <{filename}/css/page-layout.rst#breadcrumb-navigation>`__
documentation) and also in page title. Breadcrumb links are taken from the
:rst:`:breadcrumb:` field, where every line is one level of hierarchy,
consisting of an internal target link (which gets properly expanded) and title
separated by whitespace.

Example usage:

.. code:: rst

    Steam engine
    ############

    :breadcrumb: {filename}/help.rst Help
                 {filename}/help/components.rst Components

.. note-info::

    You can see the breadcrumb in action on the top and bottom of this
    documentation page (and others).

`Landing pages`_
----------------

It's possible to override the default 10-column behavior for pages to make a
`landing page <{filename}/css/page-layout.rst#landing-pages>`__ with large
cover image spanning the whole window width. Put cover image URL into a
:rst:`:cover:` field, the :rst:`:landing:` field then contains
:abbr:`reST <reStructuredText>`-processed content that appears on top of the
cover image. Contents of the :rst:`:landing:` are put into a
:html:`<div class="m-container">`, you are expected to fully take care of rows
and columns in it. The :rst:`:hide_navbar_brand:` field controls visibility of
the navbar brand link. Set it to :py:`True` to hide it, default (if not
present) is :py:`False`.

.. block-warning:: Configuration

    Currently, in order to have the :rst:`:landing:` field properly parsed, you
    need to explicitly list it in :py:`FORMATTED_FIELDS`. Don't forget that
    :py:`'summary'` is already listed there.

    .. code:: py

        FORMATTED_FIELDS += ['landing']

Example of a fully custom index page that overrides the default theme index
page (which would just list all the articles) is below. Note the overridden save
destination and URL.

.. code:: rst

    Your Brand
    ##########

    :save_as: index.html
    :url:
    :cover: {static}/static/cover.jpg
    :hide_navbar_brand: True
    :landing:
        .. container:: m-row

            .. container:: m-col-m-6 m-push-m-5

                .. raw:: html

                    <h1>Your Brand</h1>

                *This is the brand you need.*

.. block-warning:: Landing page title

    To give you full control over the landing page appearance, the page title
    is not rendered in :html:`<h1>` on top of the content as with usual pages.
    Instead you are expected to provide a heading inside the :rst:`:landing:`
    field. However, due to semantic restrictions of :abbr:`reST <reStructuredText>`,
    it's not possible to use section headers inside the :rst:`:landing:` field
    and you have to work around it using raw HTML blocks, as shown in the above
    example.

.. note-info::

    You can see the landing page in action on the `main project page <{filename}/index.rst>`_.

`Pages with cover image`_
-------------------------

Besides full-blown landing pages that give you control over the whole layout,
you can add cover images to regular pages by just specifying the :rst:`:cover:`
field but omitting the :rst:`:landing:` field. See corresponding section
`in the CSS page layout docs <{filename}/css/page-layout.rst#pages-with-cover-image>`_
for details about how the cover image affects page layout.

.. note-info::

    Real-world example of a page with cover image can be seen on the
    `Magnum Engine website <https://magnum.graphics/features/extensions/>`_.

`Page header and footer`_
-------------------------

It's possible to add extra :abbr:`reST <reStructuredText>`-processed content
(such as page-specific navigation) before and after the page contents by
putting it into :rst:`:header:` / :rst:`:footer:` fields. Compared to having
these directly in page content, these will be put semantically outside the page
:html:`<article>` element (so even before the :html:`<h1>` heading and after
the last :html:`<section>` ends). The header / footer is put, equivalently to
page content, in the center 10 columns on large screens and spanning the full
12 columns elsewhere; the container is marked as `inflatable`_. Example of a
page-specific footer navigation, extending the breadcrumb navigation from
above:

.. code:: rst

    Steam engine
    ############

    :breadcrumb: {filename}/help.rst Help
                 {filename}/help/components.rst Components
    :footer:
        `« Water tank <{filename}/help/components/water-tank.rst>`_ |
        `Components <{filename}/help/components.rst>`_ |
        `Chimney » <{filename}/help/components/chimney.rst>`_

.. block-warning:: Configuration

    Similarly to landing page content, in order to have the :rst:`:header:` /
    :rst:`:footer:` fields properly parsed, you need to explicitly list them in
    :py:`FORMATTED_FIELDS`. Don't forget that :py:`'summary'` is already listed
    there.

    .. code:: py

        FORMATTED_FIELDS += ['header', 'footer']

.. note-warning::

    The :rst:`:header:` field is not supported on `landing pages`_. In case
    both :rst:`:landing:` and :rst:`:header:` is present, :rst:`:header:` is
    ignored. However, it works as expected if just :rst:`:cover:` is present.

`News on index page`_
---------------------

If you override the index page to a custom landing page, by default you lose
the list of latest articles. That might cause the website to appear stale when
you update just the blog. In order to fix that, it's possible to show a block
with latest articles on the index page using the :py:`M_NEWS_ON_INDEX` setting.
It's a tuple of :py:`(title, count)` where :py:`title` is the block header
title that acts as a link to :py:`M_BLOG_URL` and :py:`count` is the max number
of articles shown. Example configuration:

.. code:: py

    M_NEWS_ON_INDEX = ("Latest news on our blog", 3)

.. note-success::

    You can see how this block looks on the Magnum Engine main page:
    https://magnum.graphics

`(Social) meta tags for pages`_
-------------------------------

Every page has :html:`<link rel="canonical">` pointing to its URL to avoid
duplicates in search engines when using GET parameters. In addition to the
global meta tags described in `(Social) meta tags`_ above, you can use the
:rst:`:description:` field to populate :html:`<meta name="description">`. Other
than that, the field does not appear anywhere on the rendered page. If such
field is not set, the description :html:`<meta>` tag is not rendered at all.
It's recommended to add it to :py:`FORMATTED_FIELDS` so you can make use of the
`advanced typography features <{filename}/plugins/htmlsanity.rst#typography>`_
like smart quotes etc. in it:

.. code:: py

    FORMATTED_FIELDS += ['description']

The global `Open Graph`_ and `Twitter Card`_ :html:`<meta>` tags are
specialized for pages like this:

-   Page title is mapped to ``og:title`` / ``twitter:title``
-   Page URL is mapped to ``og:url``
-   The :rst:`:summary:` field is mapped to ``og:description`` /
    ``twitter:description``. Note that if the page doesn't have explicit
    summary, Pelican takes it from the first few sentences of the content and
    that may not be what you want. This is also different from the
    :rst:`:description:` field mentioned above and, unlike with articles,
    :rst:`:summary:` doesn't appear anywhere on the rendered page.
-   The :rst:`:cover:` field (e.g. the one used on `landing pages`_), if
    present, is mapped to ``og:image`` / ``twitter:image``, overriding the
    global :py:`M_SOCIAL_IMAGE` setting. The exact same file is used without
    any resizing or cropping and is assumed to be in landscape.
-   ``twitter:card`` is set to ``summary_large_image`` if :rst:`:cover:` is
    present and to ``summary`` otherwise
-   ``og:type`` is set to ``page``

Example overriding the index page with essential properties for nice-looking
social links:

.. code:: rst

    Your Brand
    ##########

    :save_as: index.html
    :url:
    :cover: {static}/static/cover.jpg
    :summary: This is the brand you need.

.. block-success:: Recommended sizes for cover images

    Unlike the global site image described in `(Social) meta tags <#global-site-image>`_,
    page-specific cover images are assumed to be larger and in landscape to
    display large on top of the link, as they should act to promote the
    particular content instead of being just a decoration.

    `Facebook recommendations for the cover image <https://developers.facebook.com/docs/sharing/best-practices/#images>`_
    say that the image should have 1.91:1 aspect ratio and be ideally at least
    1200\ |x|\ 630 px large, while `Twitter recommends <https://developer.twitter.com/en/docs/tweets/optimize-with-cards/overview/summary-card-with-large-image>`_ 2:1 aspect ratio and at
    most 4096\ |x|\ 4096 px. In case of Twitter, the large image display is
    controlled explicitly by having ``twitter:card`` set to ``summary_large_image``,
    but for Facebook one needs to rely on their autodetection. Make sure the
    image is at least 600\ |x|\ 315 px to avoid fallback to a small thumbnail.

.. note-info::

    You can see how page links will display by pasting URL of the
    `index page <{filename}/index.rst>`_ into either `Facebook Debugger`_ or
    `Twitter Card Validator`_.

`Articles`_
===========

Compared to pages, articles have additional metadata like :rst:`:date:`,
:rst:`:author:`, :rst:`:category:` and :rst:`tags` that order them and divide
them into various sections. Besides that, there's article :rst:`:summary:`,
that, unlike with pages, is displayed in the article header; other metadata are
displayed in article footer. The article can also optionally have a
:rst:`:modified:` date, which is shown as date of last update in the footer.

All article listing pages (archives, categories, tags, authors) are displaying
just the article summary and the full article content is available only on the
dedicated article page. An exception to this is the main index or archive page,
where the first article is fully expanded so the users are greeted with some
actual content instead of just a boring list of article summaries.

Article pages show a list of sections and tags in a right sidebar. By default,
list of authors is not displayed as there is usually just one author. If you
want to display the authors as well, enable it using the :py:`M_SHOW_AUTHOR_LIST`
option in the configuration:

.. code:: py

    M_SHOW_AUTHOR_LIST = True

.. note-success::

    The theme is able to recognize additional description and images for
    authors, categories and tags from the
    `Metadata plugin <{filename}/plugins/metadata.rst>`_, if you enable it.

`Jumbo articles`_
-----------------

`Jumbo articles <{filename}/css/page-layout.rst#jumbo-articles>`__ are made
by including the :rst:`:cover:` field containing URL of the cover image.
Besides that, if the title contains an em-dash (---), it gets split into a
title and subtitle that's then rendered in a different font size. Example:

.. code:: rst

    An article --- a jumbo one
    ##########################

    :cover: {static}/static/ship.jpg
    :summary: Article summary paragraph.

    Article contents.

Sidebar with tag, category and author list shown in the classic article layout
on the right is moved to the bottom for jumbo articles. In case you need to
invert text color on cover, add a :rst:`:class:` field containing the
``m-inverted`` CSS class.

.. note-info::

    You can compare how an article with nearly the same contents looks as
    `a normal article <{filename}/examples/article.rst>`_ and a
    `jumbo article <{filename}/examples/jumbo-article.rst>`_.

`Archived articles`_
--------------------

It's possible to mark articles and archived by setting the :rst:`:archived:`
field to :py:`True`. In addition to that, you can display an arbitrary
formatted block on the article page on top of article contents right below the
summary. The content of the block is controlled by the
:py:`M_ARCHIVED_ARTICLE_BADGE` setting, containinig
:abbr:`reST <reStructuredText>`-formatted markup. The ``{year}`` placeholder,
if present, is replaced with the article year. If the setting is not present,
no block is rendered at all. Example setting:

.. code:: py

    M_ARCHIVED_ARTICLE_BADGE = """
    .. container:: m-note m-warning

        This article is from {year}. **It's old.** Deal with it.
    """

`(Social) meta tags for articles`_
----------------------------------

Every article has :html:`<link rel="canonical">` pointing to its URL to avoid
duplicates in search engines when using GET parameters. In addition to the
global meta tags described in `(Social) meta tags`_ above, you can use the
:rst:`:description:` field to populate :html:`<meta name="description">`. Other
than that, the field doesn't appear anywhere in the rendered article.  If such
field is not set, the description :html:`<meta>` tag is not rendered at all.
Again, it's recommended to add it to :py:`FORMATTED_FIELDS`.

The global `Open Graph`_ and `Twitter Card`_ :html:`<meta>` tags are
specialized for articles like this:

-   Article title is mapped to ``og:title`` / ``twitter:title``
-   Article URL is mapped to ``og:url``
-   The :rst:`:summary:` field is mapped to ``og:description`` /
    ``twitter:description``. Note that if the article doesn't have explicit
    summary, Pelican takes it from the first few sentences of the content and
    that may not be what you want. This is also different from the
    :rst:`:description:` field mentioned above.
-   The :rst:`:cover:` field from `jumbo articles`_, if present, is mapped to
    ``og:image`` / ``twitter:image``, overriding the global :py:`M_SOCIAL_IMAGE`
    setting. The exact same file is used without any resizing or cropping and
    is assumed to be in landscape. See `(Social) meta tags for pages`_ above
    for image size recommendations.
-   ``twitter:card`` is set to ``summary_large_image`` if :rst:`:cover:` is
    present and to ``summary`` otherwise
-   ``og:type`` is set to ``article``

.. note-success::

    Additional social meta tags (such as author or category info) are be
    exposed by the `Metadata plugin <{filename}/plugins/metadata.rst>`_.

.. note-info::

    You can see how article links will display by pasting
    URL of e.g. the `jumbo article`_ into either `Facebook Debugger`_ or
    `Twitter Card Validator`_.

`Controlling article appearance`_
---------------------------------

By default, the theme assumes that you provide an explicit :rst:`:summary:`
field for each article. The summary is then displayed on article listing page
and also prepended to fully expanded article. If your :rst:`:summary:` is
automatically generated by Pelican or for any other reason repeats article
content, it might not be desirable to show it in combination with article
content. This can be configured via the following setting:

.. code:: py

    M_HIDE_ARTICLE_SUMMARY = True

There's also a possibility to control this on a per-article basis by setting
:rst:`:hide_summary:` to either :py:`True` or :py:`False`. If both global and
per-article setting is present, article-specific setting has a precedence.
Example:

.. code:: rst

    An article without explicit summary
    ###################################

    :cover: {static}/static/ship.jpg
    :hide_summary: True

    Implicit article summary paragraph.

    Article contents.

.. note-info::

    Here's the visual appearance of an `article without explicit summary <{filename}/examples/article-hide-summary.rst>`_
    and a corresponding `jumbo article <{filename}/examples/jumbo-article-hide-summary.rst>`__.

As noted above, the first article is by default fully expanded on index and
archive page. However, sometimes the article is maybe too long to be expanded
or you might want to not expand any article at all. This can be controlled
either globally using the following setting:

.. code:: py

    M_COLLAPSE_FIRST_ARTICLE = True

Or, again, on a per-article basis, by setting :rst:`:collapse_first:` to either
:py:`True` or :py:`False`. If both global and per-article setting is present,
article-specific setting has a precedence.

`Pre-defined pages`_
====================

With the default configuration above the index page is just a list of articles
with the first being expanded; the archives page is basically the same. If you
want to have a custom index page (for example a `landing page <#landing-pages>`_),
remove :py:`'index'` from the :py:`DIRECT_TEMPLATES` setting and keep just
:py:`'archives'` for the blog front page. Also you may want to enable
pagination for the archives, as that's not enabled by default:

.. code:: py

    # Defaults to ['index', 'categories', 'authors', 'archives']
    DIRECT_TEMPLATES = ['archives']

    # Defaults to {'index': None, 'tag': None, 'category': None, 'author': None}
    PAGINATED_TEMPLATES = {'archives': None, 'tag': None, 'category': None, 'author': None}

.. note-warning::

    The m.css Pelican theme doesn't provide per-year, per-month or per-day
    archive pages or category, tag, author *list* pages at the moment ---
    that's why the above :py:`DIRECT_TEMPLATES` setting omits them. List of
    categories and tags is available in a sidebar from any article or article
    listing page.

Every category, tag and author has its own page that lists corresponding
articles in a way similar to the index or archives page, but without the first
article expanded. On the top of the page there is a note stating what condition
the articles are filtered with.

.. note-info::

    See how an example `category page looks <{category}Examples>`_.

Index, archive and all category/tag/author pages are paginated based on the
:py:`DEFAULT_PAGINATION` setting --- on the bottom of each page there are link
to prev and next page, besides that there's :html:`<link rel="prev">` and
:html:`<link rel="next">` that provides the same as a hint to search engines.

`Pass-through pages`_
=====================

Besides `pages`_, `articles`_ and `pre-defined pages`_ explained above, where
the content is always wrapped with the navbar on top and the footer bottom,
it's possible to have pages with fully custom markup --- for example various
presentation slides, demos etc. To do that, set the :rst:`:template:` metadata
to ``passthrough``. While it works with :abbr:`reST <reStructuredText>`
sources, this is best combined with raw HTML input. Pelican will copy the
contents of the :html:`<body>` tag verbatim and use contents of the
:html:`<title>` element for a page title, put again in the :html:`<title>`
(*not* as a :html:`<h1>` inside :html:`<body>`). Besides that, you can specify
additional metadata using the :html:`<meta name="key" content="value" />` tags:

-   :html:`<meta name="template" content="passthrough" />` needs to be always
    present in order to make Pelican use the passthrough template.
-   :html:`<meta name="css" />`, :html:`<meta name="js" />` and
    :html:`<meta name="html_header" />` specify additional CSS files,
    JavaScript files and arbitrary HTML, similarly as with normal pages. The
    ``content`` can be multiple lines, empty lines are discarded for CSS and JS
    references. Be sure to properly escape everything.
-   :html:`<meta name="class" />` can be used to add a CSS class to the
    top-level :html:`<html>` element
-   All usual Pelican metadata like ``url``, ``slug`` etc. work here as well.

Note that at the moment, the pass-through pages do not insert any of the
(social) meta tags. Example of an *input* file for a pass-through page:

.. code:: html

    <!DOCTYPE html>
    <html lang="en">
    <head>
      <title>WebGL Demo Page</title>
      <meta name="template" content="passthrough" />
      <meta name="css" content="
        m-dark.css
        https://fonts.googleapis.com/css?family=Source+Code+Pro:400,400i,600%7CSource+Sans+Pro:400,400i,600,600i
        " />
      <meta name="js" content="webgl-demo.js" />
    </head>
    <body>
    <!-- the actual page body -->
    </body>
    </html>

`Theme properties`_
===================

The theme markup is designed to have readable, nicely indented output. The code
is valid HTML5 and should be parsable as XML.

.. note-danger::

    This is one of the main goals of this project. Please
    :gh:`report a bug <mosra/m.css/issues/new>` if it's not like that.

`Troubleshooting`_
==================

`Output is missing styling`_
----------------------------

If you are on Windows and don't have Git symlinks enabled, empty CSS files
might get copied. The solution is either to reinstall Git with symlinks enabled
or manually copy all ``*.css`` files from ``css/`` to
``pelican-theme/static/``, replacing the broken symlinks present there.
