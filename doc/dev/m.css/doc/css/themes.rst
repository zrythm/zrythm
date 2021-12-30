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

Themes
######

:breadcrumb: {filename}/css.rst CSS
:footer:
    .. note-dim::
        :class: m-text-center

        `« Page layout <{filename}/css/page-layout.rst>`_ | `CSS <{filename}/css.rst>`_

.. role:: css(code)
    :language: css

The `m-theme-*.css <{filename}/css.rst>`_ files provide final theming for the
style. Currently, m.css provides two themes, a dark and a light one. A theme
file consists of just a set of CSS variables, which affect fonts, colors and
other properties. Together with theme file, m.css provides also a "driver file"
that includes all the other necessary CSS files except fonts via CSS
:css:`@import` statements --- so you can reference just ``m-dark.css`` instead
of four distinct files.

.. block-warning:: Browser support

    Note that :abbr:`some browsers <IE and Edge, I'm looking at you>` have
    problems with CSS variables and :css:`@import` statements. Because of that,
    the builtin themes provide a ``*.compiled.css`` version that contains a
    preprocessed version without CSS variables or :css:`@import` statements;
    which also make it smaller in total. This compiled version includes also
    the Pygments code highlighting style, all combined in one file.

    I recommend using the original files for development and switching to the
    compiled version when publishing the website.

.. contents::
    :class: m-block m-default

`Dark`_
=======

The dark theme is contained in the `m-dark.css <{filename}/css.rst>`_ (or
``m-dark.compiled.css``) file. Besides that, you need to reference also
`Source Sans Pro <https://fonts.google.com/specimen/Source+Sans+Pro>`_ font
(used for page copy) and `Source Code Pro <https://fonts.google.com/specimen/Source+Code+Pro>`_
(used for pre-formatted text and code). You can get them on Google Fonts. Full
markup including theme color (used for example by Vivaldi or Android browser)
is below:

.. code:: html
    :class: m-console-wrap

    <link rel="stylesheet" href="m-dark.css" /> <!-- or m-dark.compiled.css -->
    <link rel="stylesheet" href="https://fonts.googleapis.com/css?Source+Sans+Pro:400,400i,600,600i%7Cfamily=Source+Code+Pro:400,400i,600" />
    <meta name="theme-color" content="#22272e" />

This theme is used on this site and also on https://magnum.graphics.

`Light`_
========

The light theme is contained in the `m-light.css <{filename}/css.rst>`_ (or
``m-light.compiled.css``) file. Besides that, you need to reference also
`Libre Baskerville <https://fonts.google.com/specimen/Libre+Baskerville>`_ font
(used for page copy) and `Source Code Pro <https://fonts.google.com/specimen/Source+Code+Pro>`_
(used for pre-formatted text and code).

.. code:: html
    :class: m-console-wrap

    <link rel="stylesheet" href="m-light.css" /> <!-- or m-light.compiled.css -->
    <link rel="stylesheet" href="https://fonts.googleapis.com/css?family=Libre+Baskerville:400,400i,700,700i%7CSource+Code+Pro:400,400i,600" />
    <meta name="theme-color" content="#cb4b16" />

If you want to see this theme live, go to http://blog.mosra.cz.

`Make your own`_
================

Making your own theme is usually just a matter of modifying CSS variables in
the theme file. To give an example, a portion of the ``m-theme-dark.css`` file
looks like this:

.. include:: ../../../css/m-theme-dark.css
    :code: css
    :start-line: 24
    :end-line: 48

The project also bundles a Python script for *post*\ processing the CSS files
into a single ``*.compiled.css`` file without :css:`@import` statements or
variables, if you need. Download it here:

-   :gh:`postprocess.py <mosra/m.css$master/css/postprocess.py>`

Its usage is simple --- just call it with the files you want to compile
together and it will create a ``*.compiled.css`` file in the same directory:

.. code:: sh

    cd css
    ./postprocess.py m-dark.css # Creates a m-dark.compiled.css file

`Modifying the Pygments higlighting style`_
-------------------------------------------

If you want to modify the Pygments style, it's a bit more involved. You need to
edit the ``*.py`` file instead of the ``*.css``:

-   :gh:`pygments-dark.py <mosra/m.css$master/css/pygments-dark.py>`
-   :gh:`pygments-console.py <mosra/m.css$master/css/pygments-console.py>`

After making changes, copy it somewhere so Pygments can load it as a style and
then generate a CSS file out of it:

.. code:: sh

    sudo cp pygments-dark.py /usr/lib/python3.8/site-packages/pygments/styles/dark.py
    pygmentize -f html -S dark -a .m-code > pygments-dark.css

    sudo cp pygments-console.py /usr/lib/python3.8/site-packages/pygments/styles/console.py
    pygmentize -f html -S console -a .m-console > pygments-console.css

Alternatively, you can use any of the builtin styles --- pick the one you like
at http://pygments.org/demo/ and then tell ``pygmentize`` to generate a CSS for
it. For example, the below command is for the ``pastie`` style. Note that you
might want to remove the first two lines of the resulting CSS (setting
:css:`background-color`) as those would in most cases conflict with the style
in m.css.

.. code:: sh

    pygmentize -f html -S pastie -a .m-code > pygments-pastie.css

.. note-success::

    Made a theme and want to share it with the world? I'm happy to
    :gh:`incorporate your contributions <mosra/m.css/pulls/new>`.
