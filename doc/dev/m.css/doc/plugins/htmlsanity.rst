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

HTML sanity
###########

:breadcrumb: {filename}/plugins.rst Plugins
:footer:
    .. note-dim::
        :class: m-text-center

        `Plugins <{filename}/plugins.rst>`_ | `Components » <{filename}/plugins/components.rst>`_

.. role:: css(code)
    :language: css
.. role:: html(code)
    :language: html
.. role:: jinja(code)
    :language: jinja
.. role:: py(code)
    :language: py
.. role:: rst(code)
    :language: rst

Base plugin that makes your Pelican HTML output and typography look like from
the current century.

.. contents::
    :class: m-block m-default

`How to use`_
=============

`Pelican`_
----------

Download the `m/htmlsanity.py <{filename}/plugins.rst>`_ file, put it
including the ``m/`` directory into one of your :py:`PLUGIN_PATHS` and add
:py:`m.htmlsanity` package to your :py:`PLUGINS` in ``pelicanconf.py``.

.. code:: python

    PLUGINS += ['m.htmlsanity']
    M_HTMLSANITY_SMART_QUOTES = True
    M_HTMLSANITY_HYPHENATION = True

Hyphenation (see below) requires the `Pyphen <https://pyphen.org/>`_ library,
either install it via ``pip`` or your distribution package manager or disable
it with the above setting.

.. code:: sh

    pip3 install Pyphen

`Python doc theme`_
-------------------

The ``m.htmlsanity`` plugin is available always, no need to mention it
explicitly. However, the options aren't, so you might want to supply them.
The same dependencies as for `Pelican`_ apply here.

.. code:: py

    M_HTMLSANITY_SMART_QUOTES = True
    M_HTMLSANITY_HYPHENATION = True

`Doxygen theme`_
----------------

The Doxygen theme generates the HTML output directly, without docutils in the
mix, which means there's no need for this particular plugin there.

`What it does`_
===============

This plugin replaces the default Pelican HTML4/CSS1 writer (it already sounds
horrible, right?) with a custom HTML5 writer derived from
``docutils.writers.html5_polyglot`` that does the following better:

-   Document sections are using HTML5 :html:`<section>` tag instead of
    :html:`<div class="section">`
-   Images don't have the ``alt`` attribute populated with URI, if not
    specified otherwise
-   Figures are using HTML5 :html:`<figure>` tag instead of
    :html:`<div class="figure">`, figure caption is using HTML5 :html:`<figcaption>`
    instead of :html:`<p class="caption">` and figure legend is just a :html:`<span>`
    as :html:`<div>` is not allowed inside :html:`<figure>`
-   Drops *a lot of* useless classes from elements such as :html:`<div class="docutils">`
-   Makes it possible to have :html:`<a>` elements with block contents (allowed
    in HTML5)
-   Even the Docutils HTML5 writer was putting *frightening* :html:`<colgroup>`
    things into HTML tables. Not anymore.
-   Topics are using HTML5 :html:`<aside>` tag, topic headers are using
    :html:`<h3>` instead of a nondescript :html:`<div>`
-   Line blocks are simply :html:`<p>` elements with lines delimited using
    :html:`<br>`
-   The :html:`<abbr>` tag now properly includes a ``title`` attribute
-   :abbr:`reST <reStructuredText>` comments are simply ignored, instead of
    being put into :html:`<!-- -->`

Additionally, the following m.css-specific changes are done:

-   Footnotes and footnote references have the :css:`.m-footnote`
    `styling classes <{filename}/css/typography.rst#footnotes-and-footnote-references>`_
    applied
-   Links that are just URLs have :css:`.m-link-wrap` applied `to better wrap on narrow screens <{filename}/css/typography.rst#footnotes-and-footnote-references>`_.
    Note that it's also possible to apply this and other CSS classes explicitly
    with the `m.link <{filename}/plugins/links.rst#stylable-links>`_ plugin.

`Typography`_
=============

The Pelican builtin ``TYPOGRIFY`` option is using
`SmartyPants <https://daringfireball.net/projects/smartypants/>`_ for
converting ``"``, ``'``, ``---``, ``--``, ``...`` into smart double and single
quote, em-dash, en-dash and ellipsis, respectively. Unfortunately SmartyPants
have this hardcoded for just English, so one can't easily get German or
French-style quotes.

.. note-info::

    I find it hilarious that SmartyPants author complains that everyone is
    careless about web typography, but *dares to assume* that there's just the
    English quote style and nothing else.

This plugin contains a patched version of
`smart_quotes option <http://docutils.sourceforge.net/docs/user/smartquotes.html>`_
from Docutils, which is based off SmartyPants, but with proper language
awareness on top. It is applied to whole document contents and fields that are
included in the :py:`FORMATTED_FIELDS`. See for yourself:

.. code-figure::

    .. code:: rst

        .. class:: language-en

        *"A satisfied customer is the best business strategy of all"*

        .. class:: language-de

        *"Andere Länder, andere Sitten"*

        .. class:: language-fr

        *"Autres temps, autres mœurs"*

    .. class:: language-en

    *"A satisfied customer is the best business strategy of all"*

    .. class:: language-de

    *"Andere Länder, andere Sitten"*

    .. class:: language-fr

    *"Autres temps, autres mœurs"*

The default language is taken from the standard :py:`DEFAULT_LANG` option,
which defaults to :py:`'en'`, and can be also overridden on per-page or
per-article basis using the :rst:`:lang:` metadata option. This feature is
controlled by the :py:`M_HTMLSANITY_SMART_QUOTES` option, which, similarly to
the builtin :py:`TYPOGRIFY` option, defaults to :py:`False`.

.. note-warning::

    Note that due to inherent complexity of smart quotes, only paragraph-level
    language setting is taken into account, not inline language specification.

`Hyphenation`_
==============

Or word wrap. CSS has a standard way to hyphenate words, however it's quite
hard to control from a global place and I've not yet seen any browser actually
implementing that feature. Lack of word wrap is visible especially on narrow
screens of mobile devices, where there is just way too much blank space because
of long words being wrapped on new lines.

The hyphenation is done using `Pyphen <https://pyphen.org/>`_ and is applied to
whole document contents and fields that are included in the :py:`FORMATTED_FIELDS`.
All other fields including document title are excluded from hyphenation, the
same goes for literal and raw blocks and links with URL (or e-mail) as a title.
You can see it in practice in the following convoluted example, it's also
language-aware:

.. code-figure::

    .. code:: rst

        .. class:: language-en

        incomprehensibilities

        .. class:: language-de

        Bezirksschornsteinfegermeister

        .. class:: language-fr

        anticonstitutionnellement

    .. container:: m-row

        .. container:: m-col-m-2 m-push-m-3 m-col-t-4 m-nopady

            .. class:: language-en m-noindent

            incomprehensibilities

        .. container:: m-col-m-2 m-push-m-3 m-col-t-4 m-nopady

            .. class:: language-de m-noindent

            Bezirksschornsteinfegermeister

        .. container:: m-col-m-2 m-push-m-3 m-col-t-4 m-nopady

            .. class:: language-fr m-noindent

            anticonstitutionnellement

The resulting HTML code looks like this, with :html:`&shy;` added to places
that are candidates for a word break:

.. code:: html

    <p lang="en">in&shy;com&shy;pre&shy;hen&shy;si&shy;bil&shy;i&shy;ties</p>
    <p lang="de">Be&shy;zirks&shy;schorn&shy;stein&shy;fe&shy;ger&shy;meis&shy;ter</p>
    <p lang="fr">an&shy;ti&shy;cons&shy;ti&shy;tu&shy;tion&shy;nel&shy;le&shy;ment</p>

Thanks to Unicode magic this is either hidden or converted to a real hyphen and
*doesn't* break search or SEO. Similarly to smart quotes, the default language
is taken from the standard :py:`DEFAULT_LANG` option or the :rst:`:lang:`
metadata option.This feature is controlled by the :py:`M_HTMLSANITY_HYPHENATION`
option, which also defaults to :py:`False`.

.. note-success::

    Unlike smart quotes, the hyphenation works even with inline language
    specifiers, so you can have part of a paragraph in English and part in
    French and still have both hyphenated correctly.

`Jinja2 goodies`_
=================

This plugin adds a ``rtrim`` filter to Jinja. It's like the builtin ``trim``,
but working only on the right side to get rid of excessive newlines at the end.

`reST rendering`_
-----------------

It's possible to use the reST-to-HTML5 renderer from your Jinja2 template (for
example to render a custom fine print text in the footer, specified through
settings). Just pipe your variable through the ``render_rst`` filter:

.. code:: html+jinja

    <html>
      ...
      <body>
        ...
        <footer>{{ FINE_PRINT|render_rst }}</footer>
      </body>
    </html>

The filter is fully equivalent to the builtin reST rendering and the above
:py:`M_HTMLSANITY_SMART_QUOTES`, :py:`M_HTMLSANITY_HYPHENATION` and
:py:`DEFAULT_LANG` options affect it as well.

.. note-warning::

    For content coming from document metadata fields you still have to use the
    builtin :py:`FORMATTED_FIELDS` option, otherwise additional formatting will
    get lost.

`Internal link expansion`_
--------------------------

By default, link expansion works only in document content and fields that are
referenced in the :py:`FORMATTED_FIELDS` (such as article summaries). In order
to expand links in additional fields and arbitrary strings, this plugin
provides two Jinja2 filters, producing results equivalent to
`links expanded by Pelican <https://docs.getpelican.com/en/stable/content.html#linking-to-internal-content>`_.

For formatted fields, one can use the ``expand_links`` Jinja2 filter in the
template. The link expansion needs the content object (either ``article`` or
``page``) as a parameter.

.. code:: jinja

    {{ article.legal|expand_links(article) }}

If the custom field consists of just one link (for example a link to article
cover image for a social meta tag), one can use the ``expand_link`` Jinja2
filter:

.. code:: jinja

    {{ article.cover|expand_link(article) }}

With the above being in a template and with the :py:`FORMATTED_FIELDS` setting
containing the :py:`'legal'` field, a :abbr:`reST <reStructuredText>` article
making use of both fields could look like this:

.. code:: rst

    An article
    ##########

    :date: 2017-06-22
    :legal: This article is released under `CC0 {filename}/license.rst`_.
    :cover: {static}/img/article-cover.jpg

`SITEURL formatting`_
---------------------

Convenience filter replacing the common expression :jinja:`{{ SITEURL }}/{{ page.url }}`
with a formatter that makes use of `urljoin <https://docs.python.org/3/library/urllib.parse.html#urllib.parse.urljoin>`_
so it does the right thing also when dealing with absolute URLs and even when
they start with just ``//``.

For example, if :py:`SITEURL` is :py:`'https://your.site'` and you apply
``format_siteurl`` to :py:`'about/'`, then you get ``https://your.site/about/``;
but if you apply it to :py:`'https://github.com/mosra/m.css'`, then you get
just ``https://github.com/mosra/m.css``.

.. code:: jinja

    {{ page.url|format_siteurl }}

`Text hyphenation`_
-------------------

If you need to hyphenate text that was not already processed using the
hyphenation filter (for example to wrap article titles or long words in menu
items), use the ``hyphenate`` filter:

.. code:: html+jinja

    <nav>
      <ul>
        {% for title, link in LINKS %}
        <li><a href="{{ link }}">{{ title|hyphenate }}</a></li>
        {% endfor %}
      </ul>
    </nav>

The hyphenation is by default controlled by the :py:`M_HTMLSANITY_HYPHENATION`
option. If you want to control this separately, pass a boolean variable or
simply :py:`True` to the filter ``enable`` argument. The language is by default
taken from the standard :py:`DEFAULT_LANG` option, if you want to override it,
pass language name to the ``lang`` argument. You can also take the value from
:py:`article.lang` or :py:`page.lang` attributes provided by Pelican.

.. code:: jinja

    {{ title|hyphenate(enable=TEMPLATE_HYPHENATION, lang='fr_FR') }}

Sometimes, on the other hand, you might want to de-hyphenate text that was
already hyphenated, for example to avoid potential issues in :html:`<meta>`
tags. The ``dehyphenate`` filter simply removes all occurrences of :html:`&shy;`
from passed text. The ``enable`` argument works the same as with the
``hyphenate`` filter.

.. code:: html+jinja

    <html>
      <head>
        <meta name="description" content="{{ article.summary|dehyphenate|striptags|e }}" />
      </head>
      ...

`Why choose this over ...`_
===========================

There are already
`numerous <https://github.com/getpelican/pelican-plugins/tree/master/better_figures_and_images>`_
`Pelican <https://github.com/classner/better_code_samples/tree/91717a204bbd0ae4a1af6fe25ac5dd783fb4a7db>`_
`plugins <https://github.com/getpelican/pelican-plugins/tree/master/better_tables>`__
that try to do similar things, but they *attempt* to fix it using BeautifulSoup
on top of the generated HTML. That's a horrendous thing to do, so why not just
prevent the horror from happening?
