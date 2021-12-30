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

Page layout
###########

:breadcrumb: {filename}/css.rst CSS
:footer:
    .. note-dim::
        :class: m-text-center

        `« Components <{filename}/css/components.rst>`_ | `CSS <{filename}/css.rst>`_ | `Themes » <{filename}/css/themes.rst>`_

.. role:: raw-html(raw)
   :format: html

.. role:: css(code)
    :language: css
.. role:: html(code)
    :language: html
.. role:: sh(code)
    :language: sh

With `m-layout.css <{filename}/css.rst>`_, m.css provides a full-fledged whole
page layout, including top navigation bar, footer navigation, article styling,
active section highlighting and more.

.. note-success::

    The whole CSS page layout functionality is also available through a
    `m.css Pelican theme <{filename}/themes/pelican.rst>`_. Check it out!

.. contents::
    :class: m-block m-default

`Basic markup structure`_
=========================

A barebones HTML markup structure using m.css looks like below. There is the
usual preamble, with :html:`<html lang="en">` and a :html:`<meta>` tag
specifying the file encoding, which should be the first thing in :html:`<head>`.
Some browsers assume UTF-8 by default (as per the
`HTML5 standard <https://www.w3schools.com/html/html_charset.asp>`__), but some
not, so it's better to always include it. In the :html:`<head>`
element it's important to also specify that the site is responsive via the
:html:`<meta name="viewport">` tag.

The :html:`<body>` element is divided into three parts --- top navigation bar,
main page content and the footer navigation, explained below. Their meaning is
implicit, so it's not needed to put any CSS classes on these elements, but you
have to stick to the shown structure.

.. code:: html

    <!DOCTYPE html>
    <html lang="en">
      <head>
        <meta charset="UTF-8" />
        <title>Page title</title>
        <link rel="stylesheet" href="m-dark.css" />
        <meta name="viewport" content="width=device-width, initial-scale=1.0" />
      </head>
      <body>
        <header><nav>
          <!-- here comes the top navbar -->
        </nav></header>
        <main>
          <!-- here comes the main page content -->
        </main>
        <footer><nav>
          <!-- here comes the footer navigation -->
        </nav></footer>
      </body>
    </html>

`Theme color`_
==============

Some browsers (such as Vivaldi or Chrome on Android) are able to color the
tab based on page theme color. This can be specified using the following
:html:`<meta>` tag. The color shown matches the default (dark) style, see the
`CSS themes <{filename}/css/themes.rst>`_ page for colors matching other
themes.

.. code:: html

    <meta name="theme-color" content="#22272e" />

`Top navigation bar`_
=====================

The top navigation bar is linear on
`medium and larger screens <{filename}/css/grid.rst#detailed-grid-properties>`__
and hidden under a "hamburger menu" on smaller screens. It has a distinct
background that spans the whole window width, but the content is limited to
page width as defined by the grid system.

A very simple navigation bar with a homepage link and three additional menu
items is shown below.

.. code:: html

    <header><nav id="navigation">
      <div class="m-container">
        <div class="m-row">
          <a href="#" id="m-navbar-brand" class="m-col-t-9 m-col-m-none m-left-m">Your Brand</a>
          <a id="m-navbar-show" href="#navigation" title="Show navigation" class="m-col-t-3 m-hide-m m-text-right"></a>
          <a id="m-navbar-hide" href="#" title="Hide navigation" class="m-col-t-3 m-hide-m m-text-right"></a>
          <div id="m-navbar-collapse" class="m-col-t-12 m-show-m m-col-m-none m-right-m">
            <ol>
              <li><a href="#">Features</a></li>
              <li><a href="#">Showcase</a></li>
              <li><a href="#">Download</a></li>
            </ol>
          </div>
        </div>
      </div>
    </nav></header>

The :css:`#m-navbar-brand` element is positioned on the left, in the default
dark theme shown in bold and uppercase. On medium and large screens, the
contents of :css:`#m-navbar-collapse` are shown, linearly, aligned to the right.

On small and tiny screens, the :css:`#m-navbar-show` and :css:`#m-navbar-hide`
show the :raw-html:`&#9776;` glyph aligned to the right instead of
:css:`#m-navbar-collapse`. Clicking on this "hamburger menu" icon will append
either ``#navigation`` or ``#`` to the page URL, which triggers the
:css:`#m-navbar-collapse` element to be shown under as a list or hidden again.

Similarly to `headings <{filename}/css/typography.rst#headings>`_ you can wrap
a part of the :css:`#m-navbar-brand` element in a :css:`.m-thin` CSS class to
add a thinner subtitle.

.. note-info::

    You can change the :css:`#navigation` ID to a different name, if you want,
    for example for localization --- it won't do any harm to the functionality.
    Just be sure that the :html:`<a href="#navigation">` part is updated as
    well.

`Two-column navigation on small screens`_
-----------------------------------------

To save vertical space on small screens, it's possible to split the navbar
contents into two (or more) columns using standard m.css
`grid functionality <{filename}/css/grid.rst>`_. For better accessibility,
specify the start index on the second :html:`<ol>` element.

.. code:: html
    :hl_lines: 7 8 9 10 11 12 13 14 15 16 17 18 19
    :class: m-inverted

    <header><nav id="navigation">
      <div class="m-container">
        <div class="m-row">
          <a href="#" id="m-navbar-brand" class="m-col-t-9 m-col-m-none m-left-m">Your Brand</a>
          <a id="m-navbar-show" href="#navigation" title="Show navigation" class="m-col-t-3 m-hide-m m-text-right"></a>
          <a id="m-navbar-hide" href="#" title="Hide navigation" class="m-col-t-3 m-hide-m m-text-right"></a>
          <div id="m-navbar-collapse" class="m-col-t-12 m-show-m m-col-m-none m-right-m">
            <div class="m-row">
              <ol class="m-col-t-6 m-col-m-none">
                <li><a href="#">Features</a></li>
                <li><a href="#">Showcase</a></li>
                <li><a href="#">Download</a></li>
              </ol>
              <ol class="m-col-t-6 m-col-m-none" start="4">
                <li><a href="#">Blog</a></li>
                <li><a href="#">Contact</a></li>
              </ol>
            </div>
          </div>
        </div>
      </div>
    </nav></header>

`Sub-menus in the navbar`_
--------------------------

For each menu item it's also possible to add single-level sub-menu. On larger
screens the menu will be shown on hover, on small screens the sub-menu will
appear as an indented sub-list.

.. code:: html
    :hl_lines: 15 16 17 18 19 20 21
    :class: m-inverted

    <header><nav id="navigation">
      <div class="m-container">
        <div class="m-row">
          <a href="#" id="m-navbar-brand" class="m-col-t-9 m-col-m-none m-left-m">Your Brand</a>
          <a id="m-navbar-show" href="#navigation" title="Show navigation" class="m-col-t-3 m-hide-m m-text-right"></a>
          <a id="m-navbar-hide" href="#" title="Hide navigation" class="m-col-t-3 m-hide-m m-text-right"></a>
          <div id="m-navbar-collapse" class="m-col-t-12 m-show-m m-col-m-none m-right-m">
            <div class="m-row">
              <ol class="m-col-t-6 m-col-m-none">
                <li><a href="#">Features</a></li>
                <li><a href="#">Showcase</a></li>
                <li><a href="#">Download</a></li>
              </ol>
              <ol class="m-col-t-6 m-col-m-none" start="4">
                <li>
                  <a href="#">Blog</a>
                  <ol>
                    <li><a href="#">News</a></li>
                    <li><a href="#">Archive</a></li>
                  </ol>
                </li>
                <li>
                  <a href="#">Contact</a>
                </li>
              </ol>
            </div>
          </div>
        </div>
      </div>
    </nav></header>

`Active menu item highlighting`_
--------------------------------

Add :css:`#m-navbar-current` ID to the :html:`<a>` element of a menu item
that's currently active to highlight it. This works for both top-level menu
items and sub-menus. Doesn't do anything on the :css:`#m-navbar-brand` element.

.. note-success::

    See the top of the page for live example of all navbar features and view
    page source to see how it's done here. Don't forget to try to shrink your
    browser window to see its behavior in various cases.

`Brand logo`_
-------------

Add an :html:`<img>` with a logo inside the :css:`a#m-navbar-brand`. It will be
a :css:`1.75rem` square vertically centered in the navbar. `See here how it looks <{filename}/css/page-layout-test-navbar-brand-logo.html>`__.

.. code:: html
    :hl_lines: 4 5 6 7
    :class: m-inverted

    <header><nav id="navigation">
      <div class="m-container">
        <div class="m-row">
          <a href="#" id="m-navbar-brand" class="m-col-t-9 m-col-m-none m-left-m">
            <img src="brand.png" />
            Your Brand
          </a>
          <a id="m-navbar-show" href="#navigation" title="Show navigation" class="m-col-t-3 m-hide-m m-text-right"></a>
          <a id="m-navbar-hide" href="#" title="Hide navigation" class="m-col-t-3 m-hide-m m-text-right"></a>
          ...
        </div>
      </div>
    </nav></header>

`Link back to main site from a subsite`_
----------------------------------------

If you have a subsite with independent top navbar (for example a main site and
a forum or documentation subsite), you may want to prominently show its
relation to the main site and link back to the main site as well as the subsite
homepage. The markup looks like in the following snippet (note the
:css:`#m-navbar-brand` is now a span containing two links and a "breadcrumb"
separator), `see here how it looks <{filename}/css/page-layout-test-navbar-subsite.html>`__. The `brand logo`_ works here as well if you put it inside the
:html:`<a>`.

.. code:: html
    :hl_lines: 4 5 6 7 8
    :class: m-inverted

    <header><nav id="navigation">
      <div class="m-container">
        <div class="m-row">
          <span id="m-navbar-brand" class="m-col-t-9 m-col-m-none m-left-m">
            <a href="/">Your Brand</a>
            <span class="m-breadcrumb">|</span>
            <a href="#" class="m-thin">subsite</a>
          </span>
          <a id="m-navbar-show" href="#navigation" title="Show navigation" class="m-col-t-3 m-hide-m m-text-right"></a>
          <a id="m-navbar-hide" href="#" title="Hide navigation" class="m-col-t-3 m-hide-m m-text-right"></a>
          ...
        </div>
      </div>

`Footer navigation`_
====================

The :html:`<footer>` has a slightly different background color to separate
itself from the main page content, slightly dimmer text color and smaller font
size and is padded from top and bottom by :css:`1rem` to make it feel less
crowded. It's meant to be used for navigation, but besides that it gives you a
complete freedom. As an example, you can populate it with four columns (which
become two columns on narrow screens) of navigation and a fine print, using
just the builtin m.css grid features:

.. code:: html

    <footer><nav>
      <div class="m-container">
        <div class="m-row">
          <div class="m-col-s-3 m-col-t-6">
            <h3><a href="#">Your Brand</a></h3>
            <ul>
              <li><a href="#">Features</a></li>
              <li><a href="#">Showcase</a></li>
            </ul>
          </div>
          <div class="m-col-s-3 m-col-t-6">
            <h3><a href="#">Download</a></h3>
            <ul>
              <li><a href="#">Packages</a></li>
              <li><a href="#">Source</a></li>
            </ul>
          </div>
          <div class="m-clearfix-t"></div>
          <div class="m-col-s-3 m-col-t-6">
            <h3>Contact</h3>
            <ul>
              <li><a href="mailto:you@your.brand">E-mail</a></li>
              <li><a href="https://github.com/your-brand">GitHub</a></li>
            </ul>
          </div>
          <div class="m-col-s-3 m-col-t-6">
            <h3><a href="#">Blog</a></h3>
            <ul>
              <li><a href="#">News</a></li>
              <li><a href="#">Archive</a></li>
            </ul>
          </div>
        </div>
        <div class="m-row">
          <div class="m-col-l-10 m-push-l-1">
            <p>Your Brand. Copyright &copy; <a href="mailto:you@your.brand">You</a>,
            2017. All rights reserved.</p>
          </div>
        </div>
      </div>
    </nav></footer>

.. note-info::

    See the bottom of the page for a live example of footer navigation.

`Main content`_
===============

The :html:`<main>` content is separated from the header and footer by
:css:`1rem` padding, besides that there is no additional implicit styling. It's
recommended to make use of m.css `grid features <{filename}/css/grid.rst>`_ for
content layout --- in particular, the :html:`<main>` element by itself doesn't
even put any width restriction on the content.

To follow HTML5 semantic features, m.css expects you to put your main page
content into an :html:`<article>` element, be it an article or not. Heading is
always in an :html:`<h1>` inside the article element, sub-sections are wrapped
in nested :html:`<section>` elements with :html:`<h2>` and further. Example
markup together with 10-column grid setup around the main content:

.. code:: html

    <main><div class="m-container">
      <div class="m-row">
        <article class="m-col-m-10 m-push-m-1">
          <h1>A page</h1>
          <p>Some introductionary paragraph.</p>
          <section>
            <h2>Features</h2>
            <p>Section providing feature overview.</p>
          </section>
          <section>
            <h2>Pricing</h2>
            <p>Information about product pricing.</p>
          </section>
        </article>
      </div>
    </div></main>

`Landing pages`_
----------------

Besides usual pages, which have the :html:`<article>` element filled with
:html:`<h1>` followed by a wall of content, m.css has first-class support for
landing pages. The major component of a landing page is a cover image in the
background, spanning the whole page width in a :css:`#m-landing-image` element.
The image is covered by :css:`#m-landing-cover` element that blends the image
into the background on the bottom. On top of it you have full freedom to put
any layout you need, for example a logo, a short introductionary paragraph and
a download button. Note that the grid setup has to only wrap the content "below
the fold", *not* the cover image.

.. code:: html

    <main><article>
      <div id="m-landing-image" style="background-image: url('landing.jpg');">
        <div id="m-landing-cover">
          <div class="m-container">
            <!-- content displayed over the cover image -->
          </div>
        </div>
      </div>
      <div class="m-container">
        <!-- content "below the fold" folows -->
      </div>
    </article></main>

The cover image always spans the whole screen width and goes also under the top
navbar. In order to make the navbar aware of the image, put a :css:`.m-navbar-landing`
CSS class on the :html:`<nav>` element --- this makes navbar dimmer with
transparent background. Usually the brand link on the left is superfluous as
the landing page repeats it in a more prominent place, to hide it put a
:css:`.m-navbar-brand-hidden` on the :css:`#m-navbar-brand` element. While the
landing page is designed to catch attention of new users, it shouldn't prevent
regular visitors from navigating the website --- because of that the top navbar
is not hidden completely and hovering it will make it more visible. This works
similarly with the hamburger menu on small screen sizes.

.. note-info::

    You can see landing page in action `on the main page <{filename}/index.rst>`_.

`Pages with cover image`_
-------------------------

If you just want slide a cover image under content of your page and don't need
to have control over what content is over the image and what under, simply put
the following markup in front of your page content --- an outer
:css:`#m-cover-image` element with background image and an inner empty
:html:`<div>` that takes care of the fade out gradient over it.

.. code:: html
    :class: m-inverted
    :hl_lines: 2 3 4

    <main>
      <div id="m-cover-image" style="background-image: url('cover.jpg');">
        <div></div>
      </div>
      <article>
        <div class="m-container">
          <!-- the whole content of your page goes here -->
        </div>
      </article>
    </main>

.. note-info::

    Real-world example of a page with cover image can be seen on the
    `Magnum Engine website <https://magnum.graphics/features/>`_.

`Breadcrumb navigation`_
------------------------

For pages that are part of a nested structure, the :html:`<h1>` element can
contain breadcrumb navigation back to pages up in the hierarchy in a
:html:`<span class="m-breadcrumb">` element. Consider this example:

.. code-figure::

    .. code:: html

        <h1>
          <span class="m-breadcrumb">
            <a href="#">Help</a> &raquo;
            <a href="#">Components</a> &raquo;
          </span>
          Steam engine
        </h1>
        <p>Page content. Lorem ipsum dolor sit amet, consectetur adipiscing elit.
        Aenean id elit posuere, consectetur magna congue, sagittis est.</p>

    .. raw:: html

        <h1>
          <span class="m-breadcrumb">
            <a href="#">Help</a> &raquo;
            <a href="#">Components</a> &raquo;
          </span>
          Steam engine
        </h1>
        <p>Page content. Lorem ipsum dolor sit amet, consectetur adipiscing elit.
        Aenean id elit posuere, consectetur magna congue, sagittis est.</p>

`Clickable sections`_
---------------------

Using the :html:`<section>` elements gives you one advantage --- it gives you
the foundation that makes linking to particular article sections possible.
Consider the following code snippet:

.. code:: html
    :hl_lines: 4 5 8 9
    :class: m-inverted

    <article>
      <h1>A page</h1>
      <p>Some introductionary paragraph.</p>
      <section id="features">
        <h2><a href="#features">Features</a></h2>
        <p>Section providing feature overview.</p>
      </section>
      <section id="pricing">
        <h2><a href="#pricing">Pricing</a></h2>
        <p>Information about product pricing.</p>
      </section>
    </article>

Clicking on either the "Features" or "Pricing" heading will give the user a
direct link to given section and the section will be highlighed accordingly.
This works for nested sections as well.

.. note-success::

    You can observe the feature on this very page --- just click on any header
    and see how the corresponding section gets highlighted.

`Articles`_
-----------

For blog-like articles, m.css provides styling for article header, summary and
footer --- just put :html:`<header>` and :html:`<footer>` elements directly
into the surrounding :html:`<article>` tag. Article header is rendered in a
bigger and brighter font, while footer is rendered in a smaller and dimmer
font. Example markup and corresponding rendering:

.. code-figure::

    .. code:: html

        <article>
          <header>
            <h1><a href="#" rel="bookmark" title="Permalink to An article">
              <time class="m-date" datetime="2017-09-08T00:00:00+02:00">
              Sep <span class="m-date-day">8</span> 2017
              </time>
              An article
            </a></h1>
            <p>Article summary paragraph. Lorem ipsum dolor sit amet, consectetur
            adipiscing elit. Aenean id elit posuere, consectetur magna congue, sagittis
            est.</p>
          </header>
          <p>Article contents. Pellentesque est neque, aliquet nec consectetur in,
          mattis ac diam. Aliquam placerat justo ut purus interdum, ac placerat lacus
          consequat. Mauris id suscipit mauris, in scelerisque lectus. Aenean nec nunc eu
          sem tincidunt imperdiet ut non elit. Integer nisi tellus, ullamcorper vitae
          euismod quis, venenatis eu nulla.</p>
          <footer>
            <p>Posted by <a href="#">The Author</a> on
            <time datetime="2017-09-08T00:00:00+02:00">Sep 8 2017</time>.</p>
          </footer>
        </article>

    .. raw:: html

        <article>
          <header>
            <h1><a href="#" rel="bookmark" title="Permalink to An article">
              <time class="m-date" datetime="2017-09-08T00:00:00+02:00">
              Sep <span class="m-date-day">8</span> 2017
              </time>
              An article
            </a></h1>
            <p>Article summary paragraph. Lorem ipsum dolor sit amet, consectetur
            adipiscing elit. Aenean id elit posuere, consectetur magna congue, sagittis
            est.</p>
          </header>
          <p>Article contents. Pellentesque est neque, aliquet nec consectetur in,
          mattis ac diam. Aliquam placerat justo ut purus interdum, ac placerat lacus
          consequat. Mauris id suscipit mauris, in scelerisque lectus. Aenean nec nunc eu
          sem tincidunt imperdiet ut non elit. Integer nisi tellus, ullamcorper vitae
          euismod quis, venenatis eu nulla.</p>
          <footer>
            <p>Posted by <a href="#">The Author</a> on
            <time datetime="2017-09-08T00:00:00+02:00">Sep 8 2017</time>.</p>
          </footer>
        </article>

There's a dedicated styling for article date in the :css:`time.m-date` element
to go into :html:`<h1>` of article :html:`<header>`. For semantic purposes and
SEO it's good to include the date/time in a machine-readable format as well.
You can get this formatting via :sh:`date -Iseconds` Unix command. The same is
then repeated in article :html:`<footer>`.

It's good to include the :html:`<a rel="bookmark">` attribute in the permalink
to hint search engines about purpose of the link and then give the same via the
``title`` attribute.

.. note-info::

    You can also see `how the article looks <{filename}/examples/article.rst>`_
    on its own dedicated page.

`Jumbo articles`_
-----------------

For "jumbo" articles with a big cover image, a different layout is available.
Example markup, corresponding in content to the above article, but with a cover
image in background, is shown below. The markup is meant to be straight in
:html:`<main>` as it arranges the content by itself in the center 10 columns.
Date and author name is rendered on top left and right in front of the cover
image, the heading (and optional subheading) as well. By default, the text on
top of the cover image is rendered white, add an additional :css:`.m-inverted`
CSS class to have it black. The article contents are marked with
:css:`.m-container-inflatable` to make
`inflated nested layouts <{filename}/css/grid.rst#inflatable-nested-grid>`_
such as `image grid <{filename}/css/components.rst#image-grid>`_ possible.

.. code:: html

    <article id="m-jumbo">
      <header>
        <div id="m-jumbo-image" style="background-image: url('ship.jpg');">
          <div id="m-jumbo-cover">
            <div class="m-container">
              <div class="m-row">
                <div class="m-col-t-6 m-col-s-5 m-push-s-1 m-text-left">Sep 8 2017</div>
                <div class="m-col-t-6 m-col-s-5 m-push-s-1 m-text-right"><a href="#">An Au­thor</a></div>
              </div>
              <div class="m-row">
                <div class="m-col-t-12 m-col-s-10 m-push-s-1 m-col-m-8 m-push-m-2">
                  <h1><a href="#" rel="bookmark" title="Permalink to An Ar­ti­cle — a jum­bo one">
                    An article
                  </a></h1>
                  <h2>a jumbo one</h2>
                </div>
              </div>
            </div>
          </div>
        </div>
        <div class="m-container">
          <div class="m-row">
            <div class="m-col-m-10 m-push-m-1 m-nopady">
              <p>Article summary paragraph. Lorem ipsum dolor sit amet, consectetur
              adipiscing elit. Aenean id elit posuere, consectetur magna congue,
              sagittis est.</p>
            </div>
          </div>
        </div>
      </header>
      <div class="m-container m-container-inflatable">
        <div class="m-row">
          <div class="m-col-m-10 m-push-m-1 m-nopady">
          Article contents. Pellentesque est neque, aliquet nec consectetur in,
          mattis ac diam. Aliquam placerat justo ut purus interdum, ac placerat
          lacus consequat. Mauris id suscipit mauris, in scelerisque lectus.
          Aenean nec nunc eu sem tincidunt imperdiet ut non elit. Integer nisi
          tellus, ullamcorper vitae euismod quis, venenatis eu nulla.
          </div>
        </div>
      </div>
      <footer class="m-container">
        <div class="m-row">
          <div class="m-col-m-10 m-push-m-1 m-nopadb">
            <p>Posted by <a href="#">An Au­thor</a> on
            <time datetime="2017-09-08T00:00:00+02:00">Sep 8 2017</time>.</p>
          </div>
        </div>
      </footer>
    </article>

Similarly to `landing pages <#landing-pages>`_, the cover image of the jumbo
article always spans the whole screen width and goes below the top navbar. If
you want the navbar to be semi-transparent, put :css:`.m-navbar-cover` on the
:html:`<nav>` element. Compared to `landing pages <#landing-pages>`_ the navbar
retains semi-transparent background at all times.

.. note-info::

    See `how the jumbo article looks <{filename}/examples/jumbo-article.rst>`_.

`Category, author list and tag cloud`_
--------------------------------------

Wrap :html:`<h3>` headers and :html:`<ol>` list in :css:`nav.m-navpanel`, you
can also make use of the :css:`.m-block-bar-*` CSS class to
`make the list linear on small screen sizes <{filename}/css/typography.rst#lists-diaries>`_
and save vertical space. For a tag cloud, mark the :html:`<ul>` with
:css:`.m-tagcloud` and wrap individual :html:`<li>` in :css:`.m-tag-1` to
:css:`.m-tag-5` CSS classes to scale them from smallest to largest.

.. note-warning::

    The tag cloud has currently hardcoded exactly five steps.

.. code-figure::

    .. code:: html

        <nav class="m-navpanel">
          <h3>Categories</h3>
          <ol class="m-block-bar-m">
            <li><a href="#">News</a></li>
            <li><a href="#">Archive</a></li>
          </ol>
          <h3>Authors</h3>
          <ol class="m-block-bar-m">
            <li><a href="#">An Author</a></li>
            <li><a href="#">Some Other Author</a></li>
          </ol>
          <h3>Tag cloud</h3>
          <ul class="m-tagcloud">
            <li class="m-tag-1"><a href="#">Announcement</a></li>
            <li class="m-tag-5"><a href="#">C++</a></li>
            <li class="m-tag-3"><a href="#">Games</a></li>
            <li class="m-tag-4"><a href="#">Rants</a></li>
          </ul>
        </nav>

    .. raw:: html

        <nav class="m-row m-navpanel">
          <div class="m-col-s-4">
            <h3>Categories</h3>
            <ol class="m-block-bar-m">
              <li><a href="#">News</a></li>
              <li><a href="#">Archive</a></li>
            </ol>
          </div>
          <div class="m-col-s-4">
            <h3>Authors</h3>
            <ol class="m-block-bar-m">
              <li><a href="#">An Author</a></li>
              <li><a href="#">Some Other Author</a></li>
            </ol>
          </div>
          <div class="m-col-s-4">
            <h3>Tag cloud</h3>
            <ul class="m-tagcloud">
              <li class="m-tag-1"><a href="#">Announcement</a></li>
              <li class="m-tag-5"><a href="#">C++</a></li>
              <li class="m-tag-3"><a href="#">Games</a></li>
              <li class="m-tag-4"><a href="#">Rants</a></li>
            </ul>
          </div>
        </nav>

`News list on index page`_
--------------------------

Sometimes you may want just a small list of news items tucked to the bottom of
an index page that's otherwise full of other content. Mark a block with
:css:`.m-landing-news` and put a list of articles in it. The :html:`<h3>` can
be used to link to the blog front page; if you use the :html:`<time>` tag for
specifying article dates, it will be aligned to the right. Example:

.. code-figure::

    .. code:: html

        <div class="m-row">
          <div class="m-col-m-8 m-push-m-2">
            <div class="m-landing-news m-note m-default">
              <h3><a href="#">Latest news on our blog &raquo;</a></h3>
              <ul class="m-unstyled">
                <li><time class="m-text m-dim" datetime="2018-01-16T00:00:00+00:00">Jan 16, 2018</time><a href="article2.html">The latest article</a></li>
                <li><time class="m-text m-dim" datetime="2017-12-09T00:00:00+00:00">Dec 09, 2017</time><a href="article.html">A slightly older article</a></li>
              </ul>
              </div>
            </div>
          </div>
        </div>

    .. raw:: html

        <div class="m-row">
          <div class="m-col-m-8 m-push-m-2">
            <div class="m-landing-news m-note m-default">
              <h3><a href="#">Latest news on our blog &raquo;</a></h3>
              <ul class="m-unstyled">
                <li><time class="m-text m-dim" datetime="2018-01-16T00:00:00+00:00">Jan 16, 2018</time><a href="article2.html">The latest article</a></li>
                <li><time class="m-text m-dim" datetime="2017-12-09T00:00:00+00:00">Dec 09, 2017</time><a href="article.html">A slightly older article</a></li>
              </ul>
              </div>
            </div>
          </div>
        </div>
