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

Test
####

:save_as: plugins/images/test/index.html
:breadcrumb: {filename}/plugins.rst Plugins
             {filename}/plugins/images.rst Images

`Images, figures`_
==================

All images should have no ``alt`` text, unless specified manually.

Image with link:

.. image:: {static}/static/ship-small.jpg
    :target: {static}/static/ship.jpg

Image, class on top, custom alt:

.. image:: {static}/static/ship.jpg
    :class: m-fullwidth
    :alt: A Ship

Image with link, class on top:

.. image:: {static}/static/ship.jpg
    :target: {static}/static/ship.jpg
    :class: m-fullwidth

Figure with link and only a caption:

.. figure:: {static}/static/ship-small.jpg
    :target: {static}/static/ship.jpg

    A Ship

Figure with link and class on top:

.. figure:: {static}/static/ship-small.jpg
    :target: {static}/static/ship.jpg
    :figclass: m-fullwidth

    A Ship

Image grid, not inflated:

.. image-grid::

    {static}/static/ship.jpg
    {static}/static/flowers.jpg

Image grid, inflated:

.. container:: m-container-inflated

    .. image-grid::

        {static}/static/flowers.jpg
        {static}/static/ship.jpg
