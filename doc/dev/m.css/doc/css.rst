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

CSS
###

.. role:: css(code)
    :language: css
.. role:: html(code)
    :language: html

The CSS style is the essence of m.css. It makes use of HTML5 tags as much as
possible to avoid redundant classes. Contrary to other popular frameworks, all
custom CSS classes and IDs are prefixed with ``m-`` to avoid conflicts with 3rd
party styles. All sizes, paddings and border widths are specified using ``rem``
units, relative to base page font size; :css:`box-sizing: border-box` is
applied to all elements by default.

`Quick start`_
==============

To take full advantage of m.css, you need the following files written in plain
CSS. Download them below or :gh:`grab the whole Git repository <mosra/m.css>`:

-   :gh:`m-grid.css <mosra/m.css$master/css/m-grid.css>` with optional
    :gh:`m-debug.css <mosra/m.css$master/css/m-debug.css>` for responsiveness
-   :gh:`m-components.css <mosra/m.css$master/css/m-components.css>` for
    typography and basic components
-   :gh:`m-layout.css <mosra/m.css$master/css/m-layout.css>` for overall page
    and article layout
-   :gh:`m-theme-dark.css <mosra/m.css$master/css/m-theme-dark.css>` or
    :gh:`m-theme-light.css <mosra/m.css$master/css/m-theme-light.css>` for
    theme setup
-   :gh:`m-dark.css <mosra/m.css$master/css/m-dark.css>` or
    :gh:`m-light.css <mosra/m.css$master/css/m-light.css>` that :css:`@import`
    the above files for a convenient single-line referencing

Scroll below for a detailed functionality description of each file. In addition
to the above, if you want to present highlighted code snippets (or colored
terminal output) on your website, there's also a builtin style for
`Pygments <http://pygments.org/>`_, matching m.css themes:

-   :gh:`pygments-dark.css <mosra/m.css$master/css/pygments-dark.css>`,
    generated from :gh:`pygments-dark.py <mosra/m.css$master/css/pygments-dark.py>`
-   :gh:`pygments-console.css <mosra/m.css$master/css/pygments-console.css>`,
    generated from :gh:`pygments-console.py <mosra/m.css$master/css/pygments-console.py>`

Once you have the files, reference them from your HTML markup. The top-level
``m-dark.css`` / ``m-light.css`` file includes the others via a CSS
:css:`@import` statement, so you don't need to link all of them. The dark theme
uses the `Source Sans Pro <https://fonts.google.com/specimen/Source+Sans+Pro>`_
font for copy and `Source Code Pro <https://fonts.google.com/specimen/Source+Code+Pro>`_
font for pre-formatted text and code, which you need to reference as well. See
the `Themes <{filename}/css/themes.rst>`_ page for requirements of other
themes.

Besides that, in order to have devices recognize your website properly as
responsive and not zoom it all the way out to an unreadable mess, don't forget
to include a proper :html:`<meta>` tag. The HTML5 DOCTYPE is also required.

.. code:: html

    <!DOCTYPE html>
    <html>
      <head>
        <link rel="stylesheet" href="m-dark.css" />
        <link rel="stylesheet" href="https://fonts.googleapis.com/css?family=Source+Code+Pro:400,400i,600%7CSource+Sans+Pro:400,400i,600&amp;subset=latin-ext" />
        <meta name="viewport" content="width=device-width, initial-scale=1.0" />
      </head>
      ...
    </html>

.. block-warning:: Browser compatibility

    Note that some older browsers have problems with CSS variables and
    :css:`@import` statements. Because of that, the builtin themes provide
    a ``*.compiled.css`` versions that are *post*\ processed without CSS
    variables or :css:`@import` statements. The compiled version includes also
    the code and console Pygments style, all combined in one file:

    -   :gh:`m-dark.compiled.css <mosra/m.css$master/css/m-dark.compiled.css>`
        (:filesize:`{static}/../css/m-dark.compiled.css`,
        :filesize-gz:`{static}/../css/m-dark.compiled.css` compressed)
    -   :gh:`m-light.compiled.css <mosra/m.css$master/css/m-light.compiled.css>`
        (:filesize:`{static}/../css/m-light.compiled.css`,
        :filesize-gz:`{static}/../css/m-light.compiled.css` compressed)

    I recommend using the original files for development and switching to the
    compiled versions when publishing the website.

.. block-info:: Tip: server-side compression

    Even though the CSS files are already quite small, enabling server-side
    compression will make your website load even faster. If you have an Apache
    server running, enabling the compression is just a matter of adding the
    following to your ``.htaccess`` file:

    .. code:: apache

        AddOutputFilter DEFLATE html css js

With this, you can start using the framework right away. Click the headings
below to get to know more.

`Grid system » <{filename}/css/grid.rst>`_
==========================================

The ``m-grid.css`` file provides a 12-column layout, inspired by
`Bootstrap <https://getbootstrap.com>`_. It provides a simple, easy-to-use
solution for modern responsive web development. It comes together with
``m-debug.css`` that helps debugging the most common mistakes in grid layouts.

`Typography » <{filename}/css/typography.rst>`_
===============================================

Sane default style for body text, paragraphs, lists, links, headings and other
common typographical elements, provided by the ``m-components.css`` file.

`Components » <{filename}/css/components.rst>`_
===============================================

The ``m-components.css`` file also contains styles for visual elements that add
more structure to your content. From simple notes and topic blocks, tables,
images or figures to complex elements like code snippets, math formulas or
image grid.

`Page layout » <{filename}/css/page-layout.rst>`_
=================================================

In ``m-layout.css`` there's a styling for the whole page including navigation
--- header and footer, section headings, article styling with sidebar, tag
cloud, active section highlighting and more.

`Themes » <{filename}/css/themes.rst>`_
=======================================

Finally, ``m-theme-dark.css`` and ``m-theme-light.css`` use CSS variables to
achieve easy theming. Two builtin themes, used by the author himself on a bunch
of websites to guarantee that everything fits well together.
