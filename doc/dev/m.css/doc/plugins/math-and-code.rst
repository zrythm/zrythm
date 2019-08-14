..
    This file is part of m.css.

    Copyright © 2017, 2018, 2019 Vladimír Vondruš <mosra@centrum.cz>

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

Math and code
#############

:breadcrumb: {filename}/plugins.rst Plugins
:footer:
    .. note-dim::
        :class: m-text-center

        `« Images <{filename}/plugins/images.rst>`_ | `Plugins <{filename}/plugins.rst>`_ | `Plots and graphs » <{filename}/plugins/plots-and-graphs.rst>`_

.. role:: css(code)
    :language: css
.. role:: html(code)
    :language: html
.. role:: ini(code)
    :language: ini
.. role:: py(code)
    :language: py
.. role:: rst(code)
    :language: rst
.. role:: sh(code)
    :language: sh

These plugins use external libraries to produce beautiful math and code
rendering directly from your :abbr:`reST <reStructuredText>` sources.

.. contents::
    :class: m-block m-default

`Math`_
=======

For Pelican, download the `m/math.py and latex2svg.py <{filename}/plugins.rst>`_
files, put them including the ``m/`` directory into one of your
:py:`PLUGIN_PATHS` and add :py:`m.math` package to your :py:`PLUGINS` in
``pelicanconf.py``. This plugin assumes presence of
`m.htmlsanity <{filename}/plugins/htmlsanity.rst>`_.

.. note-danger::

    Note that this plugin, unlike most of the others, requires at least Python
    3.5 to run properly.

.. code:: python

    PLUGINS += ['m.htmlsanity', 'm.math']
    M_MATH_RENDER_AS_CODE = False
    M_MATH_CACHE_FILE = 'm.math.cache'

For the Doxygen theme, this feature is builtin. Use either the ``@f[`` command
for block-level math or the ``@f$`` command for inline math. It's possible to
add extra CSS classes by placing ``@m_class`` in a paragraph before the actual
math block (or right before inline math), see the
`Doxygen theme-specific commands <http://localhost:8000/documentation/doxygen/#theme-specific-commands>`_
for more information. The :ini:`M_MATH_CACHE_FILE` option is supported as well;
there's no equivalent to the :ini:`M_MATH_RENDER_AS_CODE` option implemented at
this point.

In addition you need some LaTeX distribution installed. Use your distribution
package manager, for example on Ubuntu:

.. code:: sh

    sudo apt install \
        texlive-base \
        texlive-latex-extra \
        texlive-fonts-extra \
        texlive-fonts-recommended

.. block-warning:: Note for macOS users

    On macOS 10.11 and up, if you use MacTex, there's some additional setup
    needed `as detailed here <https://tex.stackexchange.com/a/249967>`_ --- you
    need to update your :sh:`$PATH` so the binaries are properly found:

    .. code:: sh

        export PATH=$PATH:/Library/TeX/Distributions/.DefaultTeX/Contents/Programs/texbin

.. note-success::

    This plugin makes use of the ``latex2svg.py`` utility from :gh:`tuxu/latex2svg`,
    © 2017 `Tino Wagner <http://www.tinowagner.com/>`_, licensed under
    :gh:`MIT <tuxu/latex2svg$master/LICENSE.md>`.

The plugin overrides the builtin docutils
`math directive <http://docutils.sourceforge.net/docs/ref/rst/directives.html#math>`_
and `math interpreted text role <http://docutils.sourceforge.net/docs/ref/rst/roles.html#math>`_
and:

-   Instead of relying on MathML or MathJax, converts input LaTeX math formula
    to a SVG file, which is then embedded directly to the page. All glyphs are
    converted to paths.
-   Size is represented using CSS :css:`em` units so the formula follows
    surrounding text size.
-   Adds a possibility to color the whole formula or parts of it using colors
    that follow the current theme.
-   Adds a :html:`<title>` containing the original formula to the generated
    :html:`<svg>` element for accessibility.

Put `math blocks <{filename}/css/components.rst#math>`_ into the :rst:`.. math::`
directive; if you want to color the equations, add corresponding
`CSS class <{filename}/css/components.rst#colors>`_ via a :rst:`:class:`
option.

.. code-figure::

    .. code:: rst

        .. math::
            :class: m-success

            \boldsymbol{A} = \begin{pmatrix}
                \frac{2n}{s_x} & 0 & 0 & 0 \\
                0 & \frac{2n}{s_y} & 0 & 0 \\
                0 & 0 & \frac{n + f}{n - f} & \frac{2nf}{n - f} \\
                0 & 0 & -1 & 0
            \end{pmatrix}

    .. math::
        :class: m-success

        \boldsymbol{A} = \begin{pmatrix}
            \frac{2n}{s_x} & 0 & 0 & 0 \\
            0 & \frac{2n}{s_y} & 0 & 0 \\
            0 & 0 & \frac{n + f}{n - f} & \frac{2nf}{n - f} \\
            0 & 0 & -1 & 0
        \end{pmatrix}

Inline math can be wrapped in the :rst:`:math:` interpreted text role. If you
want to add additional CSS classes, derive a custom role from it.

.. code-figure::

    .. code:: rst

        .. role:: math-info(math)
            :class: m-info

        Quaternion-conjugated dual quaternion is :math-info:`\hat q^* = q_0^* + q_\epsilon^*`,
        while dual-conjugation gives :math:`\overline{\hat q} = q_0 - \epsilon q_\epsilon`.

    .. role:: math-info(math)
        :class: m-info

    Quaternion-conjugated dual quaternion is :math-info:`\hat q^* = q_0^* + q_\epsilon^*`,
    while dual-conjugation gives :math:`\overline{\hat q} = q_0 - \epsilon q_\epsilon`.

The resulting SVG follows font size of surrounding text, so you can use math
even outside of main page copy:

.. code-figure::

    .. code:: rst

        .. button-success:: https://tauday.com/

            The :math:`\tau` manifesto

            they say :math:`\pi` is wrong

    .. button-success:: https://tauday.com/

        The :math:`\tau` manifesto

        they say :math:`\pi` is wrong

The ``xcolor`` package is enabled by default together with names matching CSS
color classes. You can use it to highlight different parts of the formula:

.. code-figure::

    .. code:: rst

        .. math::

            \boldsymbol{A} = \begin{pmatrix}
                {\color{m-info} s_x} & 0 & 0 & {\color{m-success} t_x} \\
                0 & {\color{m-info} s_y} & 0 & {\color{m-success} t_y} \\
                0 & 0 & {\color{m-info} s_z} & {\color{m-success} t_z} \\
                0 & 0 & 0 & 1
            \end{pmatrix}

    .. math::

        \boldsymbol{A} = \begin{pmatrix}
            {\color{m-info} s_x} & 0 & 0 & {\color{m-success} t_x} \\
            0 & {\color{m-info} s_y} & 0 & {\color{m-success} t_y} \\
            0 & 0 & {\color{m-info} s_z} & {\color{m-success} t_z} \\
            0 & 0 & 0 & 1
        \end{pmatrix}

The :py:`M_MATH_CACHE_FILE` setting (defaulting to ``m.math.cache`` in the
site root directory) describes a file used for caching rendered LaTeX math
formulas for speeding up subsequent runs. Cached output that's no longer needed
is periodically pruned and new formulas added to the file. Set it to :py:`None`
to disable caching.

.. note-info::

    LaTeX can be sometimes a real pain to set up. In order to make it possible
    to work on sites that use the :py:`m.math` plugin on machines without LaTeX
    installed, you can enable a fallback option to render all math as code
    blocks using the :py:`M_MATH_RENDER_AS_CODE` setting. That can be, for
    example, combined with a check for presence of the LaTeX binary:

    .. code:: py

        import shutil
        import logging

        if not shutil.which('latex'):
            logging.warning("LaTeX not found, fallback to rendering math as code")
            M_MATH_RENDER_AS_CODE = True

`Math figure`_
--------------

See the `m.components <{filename}/plugins/components.rst#code-math-and-graph-figure>`__
plugin for details about code figures using the :rst:`.. math-figure::`
directive.

.. code-figure::

    .. code:: rst

        .. math-figure:: Infinite projection matrix

            .. math::
                :class: m-success

                \boldsymbol{A} = \begin{pmatrix}
                    \frac{2n}{s_x} & 0 & 0 & 0 \\
                    0 & \frac{2n}{s_y} & 0 & 0 \\
                    0 & 0 & -1 & -2n \\
                    0 & 0 & -1 & 0
                \end{pmatrix}

            With :math:`f = \infty`.

    .. math-figure:: Infinite projection matrix

        .. math::
            :class: m-success

            \boldsymbol{A} = \begin{pmatrix}
                \frac{2n}{s_x} & 0 & 0 & 0 \\
                0 & \frac{2n}{s_y} & 0 & 0 \\
                0 & 0 & -1 & -2n \\
                0 & 0 & -1 & 0
            \end{pmatrix}

        .. class:: m-noindent

        With :math:`f = \infty`.

`Code`_
=======

For Pelican, download the `m/code.py and ansilexer.py <{filename}/plugins.rst>`_
files, put them including the ``m/`` directory into one of your :py:`PLUGIN_PATHS`
and add :py:`m.code` package to your :py:`PLUGINS` in ``pelicanconf.py``. This
plugin assumes presence of `m.htmlsanity <{filename}/plugins/htmlsanity.rst>`_.

.. code:: python

    PLUGINS += ['m-htmlsanity', 'm.code']

For the Doxygen theme, this feature is builtin. Use the ``@code{.ext}`` command
either in a block or inline, the various ``@include`` and ``@snippet`` commands
support it as well. Language detection is done from the value of ``.ext`` in
the ``@code`` command and from the include/snippet filename for the others.
It's possible to add extra CSS classes by placing ``@m_class`` in a paragraph
before the actual code block (or right before inline code), see the
`Doxygen theme-specific commands <http://localhost:8000/documentation/doxygen/#theme-specific-commands>`_
for more information. There's no possibility to highlight particular code
lines.

In addition you need to have `Pygments <http://pygments.org>`_ installed. Get
it via ``pip`` or your distribution package manager:

.. code:: sh

    pip3 install Pygments

The plugin overrides the builtin docutils
`code directive <http://docutils.sourceforge.net/docs/ref/rst/directives.html#code>`_
and `code interpreted text role <http://docutils.sourceforge.net/docs/ref/rst/roles.html#code>`_,
replaces `Pelican code-block directive <https://docs.getpelican.com/en/stable/content.html#syntax-highlighting>`_ and:

-   Wraps Pygments output in :html:`<code>` element for inline code and
    :html:`<pre>` element for code blocks with :css:`.m-code` CSS class
    applied.
-   Removes useless CSS classes from the output.

Put `code blocks <{filename}/css/components.rst#code>`_ into the :rst:`.. code::`
directive and specify the language via a parameter. Use :rst:`:hl_lines:`
option to highlight lines; if you want to add additional CSS classes, use the
:rst:`:class:` option.

.. code-figure::

    .. code:: rst

        .. code:: c++
            :hl_lines: 4 5
            :class: m-inverted

            #include <iostream>

            int main() {
                std::cout << "Hello world!" << std::endl;
                return 0;
            }

    .. code:: c++
        :hl_lines: 4 5
        :class: m-inverted

        #include <iostream>

        int main() {
            std::cout << "Hello world!" << std::endl;
            return 0;
        }

The builtin `include directive <http://docutils.sourceforge.net/docs/ref/rst/directives.html#include>`_
is also patched to use the improved code directive. Simply specify external
code snippets filename and set the language using the :rst:`:code:` option.
All options of the :rst:`.. code::` directive are supported as well.

.. code-figure::

    .. code:: rst

        .. include:: snippet.cpp
            :code: c++
            :start-line: 2

    .. include:: math-and-code-snippet.cpp
        :code: c++
        :start-line: 2

.. note-info::

    Note that the :rst:`.. include::` directives are processed before Pelican
    comes into play, and thus no special internal linking capabilities are
    supported. In particular, relative paths are assumed to be relative to path
    of the source file.

For inline code highlighting, use :rst:`:code:` interpreted text role. To
specify which language should be highlighted, derive a custom role from it:

.. code-figure::

    .. code:: rst

        .. role:: cmake(code)
            :language: cmake

        .. role:: cpp(code)
            :language: cpp

        With the :cmake:`add_executable(foo bar.cpp)` CMake command you can create an
        executable from a file that contains just :cpp:`int main() { return 666; }` and
        nothing else.

    .. role:: cmake(code)
        :language: cmake

    .. role:: cpp(code)
        :language: cpp

    With the :cmake:`add_executable(foo bar.cpp)` CMake command you can create
    an executable from a file that contains just :cpp:`int main() { return 666; }`
    and nothing else.

`Colored terminal output`_
--------------------------

Use the ``ansi`` pseudo-language for highlighting
`colored terminal output <{filename}/css/components.rst#colored-terminal-output>`_.
The plugin will take care of the rest like using the custom Pygments lexer and
assigning a proper CSS class. Because ANSI escape codes might cause problems
with some editors and look confusing when viewed via :sh:`git diff` on the
terminal, it's best to have the listings in external files and use
:rst:`.. include::`:

.. code-figure::

    .. code:: rst

        .. include:: console.ansi
            :code: ansi

    .. include:: math-and-code-console.ansi
        :code: ansi
        :class: m-nopad

Apart from the basic color set there's also a very rudimentary support for
24bit colors using the ``\033[{?};2;{r};{g};{b}m`` color sequence --- currently
either just the foreground (the ``\033[38;2;`` prefix) or foreground
immediately followed by background color specification (the
``\033[48;2;`` prefix):

.. include:: math-and-code-console-colors.ansi
    :code: ansi

`Code figure`_
--------------

See the `m.components <{filename}/plugins/components.rst#code-math-and-graph-figure>`__
plugin for details about code figures using the :rst:`.. code-figure::`
directive.
