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

Plugins
#######

.. role:: py(code)
    :language: py

In addition to providing a theme for `Pelican <{filename}/themes/pelican.rst>`_
or `Doxygen <{filename}/documentation/doxygen.rst>`_  that styles the overall
page layout and basic typography, m.css also contains a collection of plugins,
extending the capabilities futher.

`Usage`_
========

For use with Pelican, each plugin is a standalone ``*.py`` file that is meant
to be downloaded and put into a ``m/`` subdirectory into one of your
:py:`PLUGIN_PATHS`. Then you add given :py:`m.something` package to your
:py:`PLUGINS` in ``pelicanconf.py`` and restart Pelican. Download the plugins
below or :gh:`grab the whole Git repository <mosra/m.css>`:

-   :gh:`m.htmlsanity <mosra/m.css$master/plugins/m/htmlsanity.py>`
-   :gh:`m.components <mosra/m.css$master/plugins/m/components.py>`
-   :gh:`m.images <mosra/m.css$master/plugins/m/images.py>`
-   :gh:`m.math  <mosra/m.css$master/plugins/m/math.py>` (needs also
    :gh:`latex2svg <mosra/m.css$master/plugins/latex2svg.py>`),
    :gh:`m.code <mosra/m.css$master/plugins/m/code.py>` (needs also
    :gh:`ansilexer <mosra/m.css$master/plugins/ansilexer.py>`)
-   :gh:`m.plots <mosra/m.css$master/plugins/m/plots.py>`,
    :gh:`m.dot <mosra/m.css$master/plugins/m/dot.py>`,
    :gh:`m.qr <mosra/m.css$master/plugins/m/qr.py>`
-   :gh:`m.link <mosra/m.css$master/plugins/m/link.py>`,
    :gh:`m.gh <mosra/m.css$master/plugins/m/gh.py>`,
    :gh:`m.dox <mosra/m.css$master/plugins/m/dox.py>`,
    :gh:`m.gl <mosra/m.css$master/plugins/m/gl.py>`,
    :gh:`m.vk <mosra/m.css$master/plugins/m/vk.py>`,
    :gh:`m.abbr <mosra/m.css$master/plugins/m/abbr.py>`,
    :gh:`m.filesize <mosra/m.css$master/plugins/m/filesize.py>`
-   :gh:`m.alias <mosra/m.css$master/plugins/m/alias.py>`
    :label-flat-primary:`pelican only`
-   :gh:`m.metadata <mosra/m.css$master/plugins/m/metadata.py>`
    :label-flat-primary:`pelican only`
-   :gh:`m.sphinx <mosra/m.css$master/plugins/m/metadata.py>`

For the `Python doc theme <{filename}/documentation/python.rst>`_ it's enough
to simply list them in :py:`PLUGINS`. For the `Doxygen theme <{filename}/documentation/doxygen.rst>`_,
all plugins that make sense in its context are implicitly exposed to it,
without needing to explicitly enable them.

Note that particular plugins can have additional dependencies, see
documentation of each of them to see more. Click on the headings below to get
to know more.

`HTML sanity » <{filename}/plugins/htmlsanity.rst>`_
====================================================

The :py:`m.htmlsanity` plugin is essential for m.css. It makes your markup
valid HTML5, offers a few opt-in typographical improvements and enables you to
make full use of features provided by other plugins.

`Components » <{filename}/plugins/components.rst>`_
===================================================

All `CSS components <{filename}/css/components.rst>`_ are exposed by the
:py:`m.components` plugin, so you can use them via :abbr:`reST <reStructuredText>`
directives without needing to touch HTML and CSS directly.

`Images » <{filename}/plugins/images.rst>`_
===========================================

Image-related CSS components are implemented by the :py:`m.images` plugin,
overriding builtin :abbr:`reST <reStructuredText>` functionality and providing
a convenient automatic way to arrange photos in an image grid.

`Math and code » <{filename}/plugins/math-and-code.rst>`_
=========================================================

The :py:`m.math` and :py:`m.code` plugins use external libraries for math
rendering and syntax highlighting, so they are provided as separate packages
that you can but don't have to use. With these, math and code snippets can be
entered directly in your :abbr:`reST <reStructuredText>` sources.

`Plots and graphs » <{filename}/plugins/plots-and-graphs.rst>`_
===============================================================

With :py:`m.plots`, :py:`m.dot` and :py:`m.qr` you can render various graphs,
charts and QR codes directly from values in your :abbr:`reST <reStructuredText>`
sources. The result is embedded as an inline SVG and can be styled using CSS
like everything else.

`Links and other » <{filename}/plugins/links.rst>`_
===================================================

The :py:`m.link`, :py:`m.gh`, :py:`m.dox`, :py:`m.gl`, :py:`m.vk`, :py:`m.abbr`,
:py:`m.filesize` and :py:`m.alias` plugins make it easy for you to link to
GitHub projects, issues or PRs, to Doxygen documentation, query file sizes and
provide URL aliases to preserve link compatibility.

`Metadata » <{filename}/plugins/metadata.rst>`_
===============================================

With the :py:`m.metadata` plugin it's possible to assign additional description
and images to authors, categories and tags. The information can then appear on
article listing page, as a badge under the article or be added to social meta
tags.

`Sphinx » <{filename}/plugins/sphinx.rst>`_
===========================================

The :py:`m.sphinx` plugin brings Sphinx-compatible directives for documenting
modules, classes and other to the `Python doc theme`_.
