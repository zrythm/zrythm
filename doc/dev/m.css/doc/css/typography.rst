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

Typography
##########

:breadcrumb: {filename}/css.rst CSS
:footer:
    .. note-dim::
        :class: m-text-center

        `« Grid <{filename}/css/grid.rst>`_ | `CSS <{filename}/css.rst>`_ | `Components » <{filename}/css/components.rst>`_

.. role:: css(code)
    :language: css
.. role:: html(code)
    :language: html
.. role:: rst(code)
    :language: rst

Right after being responsive, typography is the second most important thing in
m.css. The `m-components.css <{filename}/css.rst>`_ file styles the most often
used HTML elements to make them look great by default.

.. contents::
    :class: m-block m-default

`Paragraphs, quotes and poems`_
===============================

Each :html:`<p>` element inside :html:`<main>` has the first line indented, is
justified and is separated from the following content by some padding. The
:html:`<blockquote>` elements are indented with a distinctive line on the left.
Because the indentation may look distracting for manually wrapped line blocks,
assign :css:`.m-poem` to such paragraph to indent all lines the same way. To
remove the indentation and justification altogether, use :css:`.m-noindent`.
Spacing between lines can be extended to 150% using :css:`.m-spacing-150`.

.. code-figure::

    .. code:: html

        <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aenean id elit
        posuere, consectetur magna congue, sagittis est. Pellentesque est neque,
        aliquet nec consectetur in, mattis ac diam. Aliquam placerat justo ut purus
        interdum, ac placerat lacus consequat.</p>

        <blockquote><p>Ut dictum enim posuere metus porta, et aliquam ex condimentum.
        Proin sagittis nisi leo, ac pellentesque purus bibendum sit
        amet.</p></blockquote>

        <p class="m-poem m-spacing-150">
        Curabitur<br/>
        sodales<br/>
        arcu<br/>
        elit</p>

        <p class="m-noindent">Mauris id suscipit mauris, in scelerisque lectus. Aenean
        nec nunc eu sem tincidunt imperdiet ut non elit. Integer nisi tellus,
        ullamcorper vitae euismod quis, venenatis eu nulla.</p>

    .. raw:: html

        <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aenean id elit
        posuere, consectetur magna congue, sagittis est. Pellentesque est neque,
        aliquet nec consectetur in, mattis ac diam. Aliquam placerat justo ut purus
        interdum, ac placerat lacus consequat.</p>

        <blockquote><p>Ut dictum enim posuere metus porta, et aliquam ex condimentum.
        Proin sagittis nisi leo, ac pellentesque purus bibendum sit
        amet.</p></blockquote>

        <p class="m-poem m-spacing-150">
        Curabitur<br/>
        sodales<br/>
        arcu<br/>
        elit</p>

        <p class="m-noindent">Mauris id suscipit mauris, in scelerisque lectus. Aenean
        nec nunc eu sem tincidunt imperdiet ut non elit. Integer nisi tellus,
        ullamcorper vitae euismod quis, venenatis eu nulla.</p>

`Lists, diaries`_
=================

Ordered and unordered lists have padding on bottom only on the first level.
Mark the list with :css:`.m-unstyled` to remove the asterisks/numbers and
indentation.

.. code-figure::

    .. code:: html

        <ul>
          <li>Item 1</li>
          <li>
            Item 2
            <ol>
              <li>An item</li>
              <li>Another item</li>
            </ol>
          </li>
          <li>Item 3</li>
        </ul>

        <ol class="m-unstyled">
          <li>Item of an unstyled list</li>
          <li>Another item of an unstyled list</li>
        </ol>

    .. raw:: html

        <ul>
        <li>Item 1</li>
        <li>
          Item 2
          <ol>
            <li>An item</li>
            <li>Another item</li>
          </ol>
        </li>
        <li>Item 3</li>
        </ul>

        <ol class="m-unstyled">
          <li>Item of an unstyled list</li>
          <li>Another item of an unstyled list</li>
        </ol>

It's possible to convert a list to a single line with items separated by ``|``
or ``•`` to save vertical space on mobile devices and responsively change it
back on larger screens. Mark such list with :css:`.m-block-bar-*` or
:css:`.m-block-dot-*`:

.. code-figure::

    .. code:: html

        <ul class="m-block-bar-m">
          <li>Item 1</li>
          <li>Item 2</li>
          <li>Item 3</li>
        </ul>

        <ul class="m-block-dot-t">
          <li>Alice</li>
          <li>Bob</li>
          <li>Joe</li>
        </ul>

    .. raw:: html

        <ul class="m-block-bar-m">
          <li>Item 1</li>
          <li>Item 2</li>
          <li>Item 3</li>
        </ul>

        <ul class="m-block-dot-t">
          <li>Alice</li>
          <li>Bob</li>
          <li>Joe</li>
        </ul>

.. note-success::

    Shrink your browser window to see the effect in the above list.

Mark your definition list with :css:`.m-diary` to put the titles next to
definitions.

.. code-figure::

    .. code:: html

        <dl class="m-diary">
          <dt>07:30:15</dt>
          <dd>Woke up. The hangover is crazy today.</dd>
          <dt>13:47:45</dt>
          <dd>Got up from bed. Trying to find something to eat.</dd>
          <dt>23:34:13</dt>
          <dd>Finally put my pants on. Too late.</dd>
        </dl>

    .. raw:: html

        <dl class="m-diary">
          <dt>07:30:15</dt>
          <dd>Woke up. The hangover is crazy today.</dd>
          <dt>13:47:45</dt>
          <dd>Got up from bed. Trying to find something to eat.</dd>
          <dt>23:34:13</dt>
          <dd>Finally put my pants on. Too late.</dd>
        </dl>

The lists are compact by default, wrap item content in :html:`<p>` to make them
inflated. Paragraphs in list items are neither indented nor justified.

.. code-figure::

    .. code:: html

        <ul>
          <li>
            <p>Item 1, first paragraph.</p>
            <p>Item 1, second paragraph.</p>
          </li>
          <li>
            <p>Item 2</p>
            <ol>
              <li><p>An item</p></li>
              <li><p>Another item</p></li>
            </ol>
          </li>
          <li><p>Item 3</p></li>
        </ul>

    .. raw:: html

        <ul>
          <li>
            <p>Item 1, first paragraph.</p>
            <p>Item 1, second paragraph.</p>
          </li>
          <li>
            <p>Item 2</p>
            <ol>
              <li><p>An item</p></li>
              <li><p>Another item</p></li>
            </ol>
          </li>
          <li><p>Item 3</p></li>
        </ul>

`Headings`_
===========

The :html:`<h1>` is meant to be a page heading, thus it is styled a bit
differently --- it's bigger and has :css:`1rem` padding after. The :html:`<h2>`
to :html:`<h6>` are smaller and have just :css:`0.5rem` padding after, to be
closer to the content that follows. Wrapping part of the heading in a
:css:`.m-thin` will make it appear thinner, depending on used CSS theme.

.. code-figure::

    .. code:: html

        <h1>Heading 1 <span class="m-thin">with subtitle</span></h1>
        <h2>Heading 2 <span class="m-thin">with subtitle</span></h2>
        <h3>Heading 3 <span class="m-thin">with subtitle</span></h3>
        <h4>Heading 4 <span class="m-thin">with subtitle</span></h4>
        <h5>Heading 5 <span class="m-thin">with subtitle</span></h5>
        <h6>Heading 6 <span class="m-thin">with subtitle</span></h6>

    .. raw:: html

        <h1>Heading 1 <span class="m-thin">with subtitle</span></h1>
        <h2>Heading 2 <span class="m-thin">with subtitle</span></h2>
        <h3>Heading 3 <span class="m-thin">with subtitle</span></h3>
        <h4>Heading 4 <span class="m-thin">with subtitle</span></h4>
        <h5>Heading 5 <span class="m-thin">with subtitle</span></h5>
        <h6>Heading 6 <span class="m-thin">with subtitle</span></h6>

.. note-warning::

    Headings are styled in a slightly different way for
    `page sections <{filename}/css/page-layout.rst#main-content>`_ and
    `article headers <{filename}/css/page-layout.rst#articles>`_, clicks the
    links for more information. There is also a possibility to put
    `breadcrumb navigation <{filename}/css/page-layout.rst#breadcrumb-navigation>`_
    in the :html:`<h1>` element.

`Transitions`_
==============

Horizontal line is centered and fills 75% of the parent element. For a more
fancy transition, use :css:`.m-transition` on a paragraph.

.. code-figure::

    .. code:: html

        ...
        <hr/>
        ...
        <p class="m-transition">~ ~ ~</p>
        ...

    .. raw:: html

        <p>Vivamus dui quam, volutpat eu lorem sit amet, molestie tristique erat.
        Vestibulum dapibus est eu risus pellentesque volutpat.</p>
        <hr/>
        <p>Aenean tellus turpis, suscipit quis iaculis ut, suscipit nec magna.
        Vestibulum finibus sit amet neque nec volutpat. Suspendisse sit amet nisl in
        orci posuere mattis.</p>
        <p class="m-transition">~ ~ ~</p>
        <p> Praesent eu metus sed felis faucibus placerat ut eu quam. Aliquam convallis
        accumsan ante sit amet iaculis. Phasellus rhoncus hendrerit leo vitae lacinia.
        Maecenas iaculis dui ex, eu interdum lacus ornare sit amet.</p>

.. note-info::

    Transitions can be conveniently created with a :rst:`.. transition::`
    directive in your :abbr:`reST <reStructuredText>` markup using the
    `Pelican Components plugin <{filename}/plugins/components.rst#transitions>`_.

`Preformatted blocks`_
======================

The :html:`<pre>` element preserves your whitespace and adds a convenient
scrollbar if the content is too wide. If inside an
`inflatable nested grid <{filename}/css/grid.rst#inflatable-nested-grid>`_, it
will have negative margin to make its contents aligned with surrounding text.

.. code-figure::

    .. code:: html

        <pre>
        int main() {
            return 0;
        }
        </pre>

    .. raw:: html

        <pre>
        int main() {
            return 0;
        }
        </pre>

.. note-info::

    The Components page has additional information about
    `code block styling <{filename}/css/components.rst#code>`_.

`Footnotes and footnote references`_
====================================

Applying :css:`.m-footnote` to a link will turn it into a footnote reference
--- a superscript, wrapped in brackets. For the actual footnotes use
:html:`<dl class="m-footnote">`; :html:`<dt>` contains footnote ID and
:html:`<dd>` the footnote text. You can add a :html:`<span class="m-footnote">`
inside the :html:`<dd>` to provide styled back-references to the original text.

.. code-figure::

    .. code:: html

        <p>
          As also noted in the court case of <em>Mondays vs The Working People</em>
          <a href="#ref1" class="m-footnote" id="ref1-backref">1</a>, the transition
          between the weekend and a working day has a similar impact on overall
          happines as a transition between holidays and working days, however not as
          significant <a href="#ref2" class="m-footnote" id="ref2-backref">2</a>.
          This is a common theme of small talk conversations, together with
          weather <a href="#ref1" class="m-footnote" id="ref1-backref2">1</a>
          <a href="#ref3" class="m-footnote" id="ref3-backref">3</a>.
        </p>
        <dl class="m-footnote">
          <dt id="ref1">1.</dt>
          <dd>
            <span class="m-footnote">^ <a href="#ref1-backref">a</a>
            <a href="#ref1-backref2">b</a></span> Mondays vs The Working People,
            The Arizona Highest Court, 2019
          </dd>
          <dt id="ref2">2.</dt>
          <dd>
            <span class="m-footnote"><a href="#ref2-backref">^</a></span>
            <a href="https://garfield.com/comic/2014/05/26">Garfield; Monday,
            May 26, 2014</a>
          </dd>
          <dt id="ref3">3.</dt>
          <dd>
            <span class="m-footnote"><a href="#ref3-backref">^</a></span> From a
            conversation overheard this very morning.
          </dd>
        </dl>

    .. raw:: html

        <p>
          As also noted in the court case of <em>Mondays vs The Working People</em>
          <a href="#ref1" class="m-footnote" id="ref1-backref">1</a>, the transition
          between the weekend and a working day has a similar impact on overall
          happines as a transition between holidays and working days, however not as
          significant <a href="#ref2" class="m-footnote" id="ref2-backref">2</a>.
          This is a common theme of small talk conversations, together with
          weather <a href="#ref1" class="m-footnote" id="ref1-backref2">1</a>
          <a href="#ref3" class="m-footnote" id="ref3-backref">3</a>.
        </p>
        <dl class="m-footnote">
          <dt id="ref1">1.</dt>
          <dd>
            <span class="m-footnote">^ <a href="#ref1-backref">a</a>
            <a href="#ref1-backref2">b</a></span> Mondays vs The Working People,
            The Arizona Highest Court, 2019
          </dd>
          <dt id="ref2">2.</dt>
          <dd>
            <span class="m-footnote"><a href="#ref2-backref">^</a></span>
            <a href="https://garfield.com/comic/2014/05/26">Garfield; Monday,
            May 26, 2014</a>
          </dd>
          <dt id="ref3">3.</dt>
          <dd>
            <span class="m-footnote"><a href="#ref3-backref">^</a></span> From a
            conversation overheard this very morning.
          </dd>
        </dl>

`Text alignment and wrapping`_
==============================

Use :css:`.m-text-left`, :css:`.m-text-right` or :css:`.m-text-center` to
align text inside its parent element. Use :css:`.m-text-top`,
:css:`.m-text-middle` and :css:`.m-text-bottom` to align text vertically for
example in a table cell. See `Floating around <{filename}/css/grid.rst#floating-around>`_
in the grid system for aligning and floating blocks in a similar way.

By default, all text is wrapped according to default HTML rules. In order to
look better on very narrow screens, it's possible to use :html:`&shy;` to
hyphenate words. The :html:`<wbr/>` HTML tag does the same without rendering
any hyphens, and finally there's a :css:`.m-link-wrap` you can apply to links
with long URLs to break anywhere. Both hyphenation and link wrapping can be
done either manually on a case-by-case basis, or using the
`m.htmlsanity plugin <{filename}/plugins/htmlsanity.rst>`_, which can do both
automatically.

.. code-figure::

    .. code:: html

        in&shy;com&shy;pre&shy;hen&shy;si&shy;bil&shy;i&shy;ties

        in<wbr/>com<wbr/>pre<wbr/>hen<wbr/>si<wbr/>bil<wbr/>i<wbr/>ties

        <a href="http://…" class="m-link-wrap">
          llanfairpwllgwyngyllgogerychwyrndrobwllllantysiliogogogoch.co.uk
        </a>

    .. container:: m-row

        .. container:: m-col-m-2 m-push-m-2 m-col-t-3 m-nopady

            .. raw:: html

                in&shy;com&shy;pre&shy;hen&shy;si&shy;bil&shy;i&shy;ties

        .. container:: m-col-m-2 m-push-m-2 m-col-t-3 m-nopady

            .. raw:: html

                in<wbr/>com<wbr/>pre<wbr/>hen<wbr/>si<wbr/>bil<wbr/>i<wbr/>ties

        .. container:: m-col-m-4 m-push-m-2 m-col-t-6 m-nopady

            .. raw:: html

                <a href="http://llanfairpwllgwyngyllgogerychwyrndrobwllllantysiliogogogoch.co.uk" class="m-link-wrap">
                  llanfairpwllgwyngyllgogerychwyrndrobwllllantysiliogogogoch.co.uk
                </a>

`Inline elements`_
==================

.. code-figure::

    .. code:: html

        A <a href="#">link</a>, <em>emphasised text</em>, <strong>strong text</strong>,
        <abbr title="abbreviation">abbr</abbr> shown inside a normal text flow to
        verify that they don't break text flow. Then there is <small>small text</small>,
        <sup>super</sup>, <sub>sub</sub> and <s>that is probably all I can think of
        right now</s> oh, there is also <mark>marked text</mark> and
        <code>int a = some_code();</code>.

    .. raw:: html

        A <a href="#">link</a>, <em>emphasised text</em>, <strong>strong text</strong>,
        <abbr title="abbreviation">abbr</abbr> shown inside a normal text flow to
        verify that they don't break text flow. Then there is <small>small text</small>,
        <sup>super</sup>, <sub>sub</sub> and <s>that is probably all I can think of
        right now</s> oh, there is also <mark>marked text</mark> and
        <code>int a = some_code();</code>.

Links are underlined by default in all `builtin themes <{filename}/css/themes.rst>`_.
Adding :css:`.m-flat` to the :html:`<a>` element will remove the underline,
useful where underlines would be too distracting:

.. code-figure::

    .. code:: html

        <p class="m-text-center m-text m-dim">
          There is a <a href="#" class="m-flat">hidden</a> link.
        </p>

    .. raw:: html

        <p class="m-text-center m-text m-dim">
          There is a <a href="#" class="m-flat">hidden</a> link.
        </p>

For cases where you can't use the native HTML tags for emphasis, strong text,
strikethrough and subscript/superscript, the equivalent is available through
:css:`.m-em`, :css:`.m-strong`, :css:`.m-s`, :css:`.m-sup` and :css:`.m-sub`
CSS classes used together with :css:`.m-text`.

.. note-info::

    The Components page has additional information about
    `text styling <{filename}/css/components.rst#text>`_.

`Padding`_
==========

Block elements :html:`<p>`, :html:`<ol>`, :html:`<ul>`, :html:`<dl>`,
:html:`<blockqote>`, :html:`<pre>` and :html:`<hr>` by default have :css:`1rem`
padding on the bottom, except when they are the last child, to avoid excessive
spacing. A special case is lists --- components directly inside :html:`<li>`
elements have :css:`1rem` padding on the bottom, except when the :html:`<li>`
is last, to achieve consistent spacing for inflated lists.

The :css:`1rem` padding on the bottom can be disabled with :css:`.m-nopadb`,
similarly as with `grid layouts <{filename}/css/grid.rst#grid-padding>`_. On
the other hand, if you want to preserve it, add an empty :html:`<div></div>`
element after.
