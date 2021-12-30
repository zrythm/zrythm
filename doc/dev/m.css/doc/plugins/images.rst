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

Images
######

:breadcrumb: {filename}/plugins.rst Plugins
:footer:
    .. note-dim::
        :class: m-text-center

        `« Components <{filename}/plugins/components.rst>`_ | `Plugins <{filename}/plugins.rst>`_ | `Math and code » <{filename}/plugins/math-and-code.rst>`_

.. role:: css(code)
    :language: css
.. role:: py(code)
    :language: py
.. role:: rst(code)
    :language: rst

Gives sane defaults to images and figures and provides a way to present
beautiful image galleries.

.. contents::
    :class: m-block m-default

`How to use`_
=============

`Pelican`_
----------

Download the `m/images.py <{filename}/plugins.rst>`_ file, put it including the
``m/`` directory into one of your :py:`PLUGIN_PATHS` and add ``m.images``
package to your :py:`PLUGINS` in ``pelicanconf.py``. This plugin assumes
presence of `m.htmlsanity <{filename}/plugins/htmlsanity.rst>`_.

.. code:: python

    PLUGINS += ['m.htmlsanity', 'm.images']
    M_IMAGES_REQUIRE_ALT_TEXT = False

To use the image grid feature and image/figure :rst:`:scale:` option (see
below), in addition you need the `Pillow <https://pypi.python.org/pypi/Pillow>`_
library installed. Get it via ``pip`` or your distribution package manager:

.. code:: sh

    pip3 install Pillow

`Python doc theme`_
-------------------

Simply list the plugin in your :py:`PLUGINS`. The `m.htmlsanity`_ plugin is
available always, no need to mention it explicitly. The same dependencies as
for `Pelican`_ apply here.

.. code:: py

    PLUGINS += ['m.images']
    M_IMAGES_REQUIRE_ALT_TEXT = False

`Doxygen theme`_
----------------

The Doxygen theme makes the builtin ``@image`` command behave similarly to
the :rst:`.. image::` directive of this plugin, if you add a title to it, it
behaves like :rst:`.. figure::`. It's possible to add extra CSS classes by
placing ``@m_class`` in a paragraph before the actual image, see the
`Doxygen theme-specific commands <http://localhost:8000/documentation/doxygen/#theme-specific-commands>`_
for more information. The :rst:`.. image-grid::` functionality is not available
in the Doxygen theme.

`Images, figures`_
==================

The plugin overrides the builtin
`image <http://docutils.sourceforge.net/docs/ref/rst/directives.html#image>`__
and `figure <http://docutils.sourceforge.net/docs/ref/rst/directives.html#figure>`__
directives and:

-   Adds :css:`.m-image` / :css:`.m-figure` CSS classes to them so they have
    the expected m.css `image <{filename}/css/components.rst#images>`_ and
    `figure <{filename}/css/components.rst#figures>`_ styling.
-   Removes the :rst:`:align:` option, as this is better handled by m.css
    features and removes the redundant :rst:`:figwidth:` option (use
    :rst:`:width:` instead).
-   The original :rst:`:width:`, :rst:`:height:` and :rst:`:scale:` options are
    supported, only converted to a CSS ``style`` attribute instead of using
    deprecated HTML attributes. The width/height options take CSS units, the
    scale takes a percentage.
-   To maintain accessibility easier, makes it possible to enforce :rst:`:alt:`
    text for every image and figure by setting :py:`M_IMAGES_REQUIRE_ALT_TEXT`
    to :py:`True`.

You can add `additional CSS classes <{filename}/css/components.rst#images>`_ to
images or figures via the :rst:`:class:` or :rst:`:figclass:` options,
respectively. If you want the image or figure to be clickable, use the
:rst:`:target:` option. The alt text can be specified using the :rst:`:alt:`
option for both images and figures.

.. code-figure::

    .. code:: rst

        .. image:: flowers.jpg
            :target: flowers.jpg
            :alt: Flowers

        .. figure:: ship.jpg
            :alt: Ship

            A Ship

            Photo © `The Author <http://blog.mosra.cz/>`_

    .. container:: m-row

        .. container:: m-col-m-6

            .. image:: {static}/static/flowers-small.jpg
                :target: {static}/static/flowers.jpg

        .. container:: m-col-m-6

            .. figure:: {static}/static/ship-small.jpg

                A Ship

                Photo © `The Author <http://blog.mosra.cz/>`_

`Image grid`_
=============

Use the :rst:`.. image-grid::` directive for creating
`image grid <{filename}/css/components.rst#image-grid>`_. Directive contents
are a list of image URLs, blank lines separate grid rows. The plugin
automatically extracts size information and scales the images accordingly. The images are made clickable, the target is the image file itself.

If the image has EXIF information, properties such as aperture, shutter speed
and ISO are extracted and displayed in the caption on hover. It's also possible
to provide a custom title --- everything after the filename will be taken as
a title. If you use ``..`` as a title (a reST comment), it will disable EXIF
extraction and no title will be shown.

Example of a two-row image grid is below. Sorry for reusing the same two images
all over (I'm making it easier for myself); if you want to see a live example
with non-repeating images, head over to `my blog <http://blog.mosra.cz/cesty/mainau/>`_.

.. code:: rst

    .. image-grid::

        {static}/ship.jpg
        {static}/flowers.jpg

        {static}/flowers.jpg A custom title
        {static}/ship.jpg ..

.. image-grid::

    {static}/static/ship.jpg
    {static}/static/flowers.jpg

    {static}/static/flowers.jpg A custom title
    {static}/static/ship.jpg ..

.. note-warning::

    Unlike with the image and figure directives above, Pelican *needs* to have
    the images present on a filesystem to extract size information. It's
    advised to use the builtin *absolute* ``{static}`` or ``{attach}`` syntax
    for `linking to internal content <https://docs.getpelican.com/en/stable/content.html#linking-to-internal-content>`_.
