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

Plots and graphs
################

:breadcrumb: {filename}/plugins.rst Plugins
:footer:
    .. note-dim::
        :class: m-text-center

        `« Math and code <{filename}/plugins/math-and-code.rst>`_ | `Plugins <{filename}/plugins.rst>`_ | `Links and other » <{filename}/plugins/math-and-code.rst>`_

.. role:: dot(code)
    :language: dot
.. role:: ini(code)
    :language: ini
.. role:: py(code)
    :language: py
.. role:: rst(code)
    :language: rst
.. role:: css(code)
    :language: css

These plugin allow you to render plots, graphs or QR codes directly from data
specified inline in the page source. Similarly to `math rendering <{filename}/admire/math.rst>`_,
the graphics is rendered to a SVG that's embedded directly in the HTML markup.

.. note-danger:: Experimental features

    Please note that these plugins are highly experimental and at the moment
    created to fulfill a particular immediate need of the author. They might
    not work reliably on general input.

.. contents::
    :class: m-block m-default

`Plots`_
========

For Pelican, download the `m/plots.py <{filename}/plugins.rst>`_ file, put it
including the ``m/`` directory into one of your :py:`PLUGIN_PATHS` and add
``m.plots`` package to your :py:`PLUGINS` in ``pelicanconf.py``. For the
Python doc theme, it's enough to just list it in :py:`PLUGINS`:

.. code:: python

    PLUGINS += ['m.plots']
    M_PLOTS_FONT = 'Source Sans Pro'

This feature has no equivalent in the `Doxygen theme <{filename}/documentation/doxygen.rst>`_.

Set :py:`M_PLOTS_FONT` to a font that matches your CSS theme (it's Source Sans
Pro for `builtin m.css themes <{filename}/css/themes.rst>`_), note that you
*need to have the font installed* on your system, otherwise it will fall back
to whatever system font it finds instead (for example DejaVu Sans) and the
output won't look as expected. In addition you need the
`Matplotlib <https://matplotlib.org/>`_ library installed. Get it via ``pip``
or your distribution package manager:

.. code:: sh

    pip3 install matplotlib

The plugin produces SVG plots that make use of the
`CSS plot styling <{filename}/css/components.rst#plots>`_.

`Bar charts`_
-------------

Currently the only supported plot type is a horizontal bar chart, denoted by
:rst:`.. plot::` directive with :rst:`:type: hbar`:

.. code-figure::

    .. code:: rst

        .. plot:: Fastest animals
            :type: barh
            :labels:
                Cheetah
                Pronghorn
                Springbok
                Wildebeest
            :units: km/h
            :values: 109.4 88.5 88 80.5

    .. plot:: Fastest animals
        :type: barh
        :labels:
            Cheetah
            Pronghorn
            Springbok
            Wildebeest
        :units: km/h
        :values: 109.4 88.5 88 80.5

The multi-line :rst:`:labels:` option contain value labels, one per line. You
can specify unit label using :rst:`:units:`, particular values go into
:rst:`:values:` separated by whitespace, there should me as many values as
labels. Hovering over the bars will show the concrete value in a title.

It's also optionally possible to add error bars using :rst:`:error:` and
configure bar colors using :rst:`:colors:`. The colors correspond to m.css
`color classes <{filename}/css/components.rst#colors>`_ and you can either
use one color for all or one for each value, separated by whitespace. Bar chart
height is calculated automatically based on amount of values, you can adjust
the bar height using :rst:`:bar-height:`. Default value is :py:`0.4`. Similarly
it's possible to specify graph width using :rst:`:plot-width:`, the default
:py:`8` is tuned for a page-wide plot.

It's possible to add an extra line of labels using :rst:`:labels-extra:`.
Again, there should be as many entries as primary labels and values. To omit an
extra label for a value, specify it as the :abbr:`reST <reStructuredText>`
comment :rst:`..`.

.. code-figure::

    .. code:: rst

        .. plot:: Runtime cost
            :type: barh
            :labels:
                Ours minimal
                Ours default
                3rd party
                Full setup
            :labels-extra:
                15 modules
                60 modules
                200 modules
                ..
            :units: µs
            :values: 15.09 84.98 197.13 934.27
            :errors: 0.74 3.65 9.45 25.66
            :colors: success info danger dim
            :bar-height: 0.6

    .. plot:: Runtime cost
        :type: barh
        :labels:
            Ours minimal
            Ours default
            3rd party
            Full setup
        :labels-extra:
            15 modules
            60 modules
            200 modules
            ..
        :units: µs
        :values: 15.09 84.98 197.13 934.27
        :errors: 0.74 3.65 9.45 25.66
        :colors: success info danger dim
        :bar-height: 0.6

`Stacked values`_
-----------------

It's possible to stack several values on each other by providing a second
(third, ...) like for :rst:`:values:` (and :rst:`:errors:` as well). The values
are added together --- not overlapped --- so e.g. showing values of 20 and 40
stacked together will result in the bar being 60 units long in total. Hovering
over the stacked values will show magnitude of just given part, not the summed
value.

The :rst:`:colors:` option works for these as well, either have each line a
single value on each line to color each "slice" differently, or have one color
per value like shown above.

.. code-figure::

    .. code:: rst

        .. plot:: Download size (*.js, *.wasm)
            :type: barh
            :labels:
                Sdl2Application
                Sdl2Application
                EmscriptenApplication
            :labels-extra:
                -s USE_SDL=2
                -s USE_SDL=1
                ..
            :units: kB
            :values:
                111.9 74.4 52.1
                731.2 226.3 226.0
            :colors:
                success
                info

    .. plot:: Download size (*.js, *.wasm)
        :type: barh
        :labels:
            Sdl2Application
            Sdl2Application
            EmscriptenApplication
        :labels-extra:
            -s USE_SDL=2
            -s USE_SDL=1
            ..
        :units: kB
        :values:
            111.9 74.4 52.1
            731.2 226.3 226.0
        :colors:
            success
            info

    .. class:: m-text-center m-text m-dim m-small

    (graph source: https://blog.magnum.graphics/announcements/new-emscripten-application-implementation/)

`Graphs`_
=========

For Pelican, download the `m/dot.py <{filename}/plugins.rst>`_ file, put it
including the ``m/`` directory into one of your :py:`PLUGIN_PATHS` and add
``m.dot`` package to your :py:`PLUGINS` in ``pelicanconf.py``. For the
Python doc theme, it's enough to just list it in :py:`PLUGINS`.

.. code:: python

    PLUGINS += ['m.dot']
    M_DOT_FONT = 'Source Sans Pro'
    M_DOT_FONT_SIZE = 16.0

Set :py:`M_DOT_FONT` and :py:`M_DOT_FONT_SIZE` to a font that matches your CSS
theme (it's Source Sans Pro at :css:`16px` for
`builtin m.css themes <{filename}/css/themes.rst>`_), note that you *need to
have the font installed* on your system, otherwise it will fall back to
whatever system font it finds instead (for example DejaVu Sans) and the output
won't look as expected.

In case of Doxygen, this feature is builtin. Use the ``@dot`` and ``@dotfile``
commands. It's possible to add extra CSS classes by placing ``@m_class`` in a
paragraph before the actual graph block, see the
`Doxygen theme-specific commands <http://localhost:8000/documentation/doxygen/#theme-specific-commands>`_
for more information. Font name and size is controlled using the builtin
:ini:`DOT_FONTNAME` and :ini:`DOT_FONTSIZE` options.

In addition you need the
`Graphviz <https://graphviz.org/>`_ library installed. Get it via your
distribution package manager, for example on Ubuntu:

.. code:: sh

    sudo apt install graphviz

The plugin produces SVG graphs that make use of the
`CSS graph styling <{filename}/css/components.rst#graphs>`_.

`Directed graphs`_
--------------------

The :rst:`.. digraph::` directive uses the ``dot`` tool to produce directed
graphs. The optional directive argument is graph title, contents is whatever
you would put inside the :dot:`digraph` block. Use the :rst:`:class:` to
specify a `CSS color class <{filename}/css/components.rst#colors>`_ for the
whole graph, it's also possible to color particular nodes and edges using the
(currently undocumented) ``class`` attribute.

.. code-figure::

    .. code:: rst

        .. digraph:: Finite state machine

            rankdir=LR

            S₁ [shape=doublecircle class="m-primary"]
            S₂ [shape=circle]
            _  [style=invis]

            _  -> S₁ [class="m-warning"]
            S₁ -> S₂ [label="0"]
            S₂ -> S₁ [label="0"]
            S₁ -> S₁ [label="1"]
            S₂ -> S₂ [label="1"]

    .. digraph:: Finite state machine

        rankdir=LR

        S₁ [shape=doublecircle class="m-primary"]
        S₂ [shape=circle]
        _  [style=invis]
        b  [style=invis]

        _  -> S₁ [class="m-warning"]
        S₂ -> b  [style=invis]
        S₁ -> S₂ [label="0"]
        S₂ -> S₁ [label="0"]
        S₁ -> S₁ [label="1"]
        S₂ -> S₂ [label="1"]

For more information check the official
`GraphViz Reference <https://www.graphviz.org/doc/info/>`_, in particular the
extensive `attribute documentation <https://www.graphviz.org/doc/info/attrs.html>`_.

.. note-warning::

    Note that currently all styling is discarded and only the
    ``class`` and ``fontsize`` attributes are taken into account.

.. note-warning::

    The ``class`` attribute is new in Graphviz 2.40.1. If you have an older
    version on your system, this attribute will get ignored.

`Undirected graphs`_
--------------------

The :rst:`.. graph::` and :rst:`.. strict-graph::` directives are similar to
:rst:`.. digraph::`, but allow undirected graphs only. Again these are
equivalent to :dot:`graph` and :dot:`strict graph` in the DOT language:

.. code-figure::

    .. code:: rst

        .. graph:: A house
            :class: m-success

            { rank=same 0 1 }
            { rank=same 2 4 }

            0 -- 1 -- 2 -- 3 -- 4 -- 0 -- 2 -- 4 --1
            3 [style=solid]

    .. graph:: A house
        :class: m-success

        rankdir=BT

        { rank=same 0 1 }
        { rank=same 2 4 }

        0 -- 1 -- 2 -- 3 -- 4 -- 0 -- 2 -- 4 --1
        3 [style=filled]

`Graph figure`_
---------------

See the `m.components <{filename}/plugins/components.rst#code-math-and-graph-figure>`__
plugin for details about graph figures using the :rst:`.. graph-figure::`
directive.

.. code-figure::

    .. code:: rst

        .. graph-figure:: Impenetrable logic

            .. digraph::

                rankdir=LR
                yes [shape=circle class="m-primary" style=filled]
                no [shape=circle class="m-primary"]
                yes -> no [label="no" class="m-primary"]
                no -> no [label="no"]

            No.

    .. graph-figure:: Impenetrable logic

        .. digraph::

            rankdir=LR
            yes [shape=circle class="m-primary" style=filled]
            no [shape=circle class="m-primary"]
            yes -> no [label="no" class="m-primary"]
            no -> no [label="no"]

        .. class:: m-noindent

        No.

`QR code`_
==========

For Pelican, download the `m/qr.py <{filename}/plugins.rst>`_ file, put it
including the ``m/`` directory into one of your :py:`PLUGIN_PATHS` and add
``m.qr`` package to your :py:`PLUGINS` in ``pelicanconf.py``. For the
Python doc theme, it's enough to just list it in :py:`PLUGINS`:

.. code:: python

    PLUGINS += ['m.qr']

This feature has no equivalent in the `Doxygen theme <{filename}/documentation/doxygen.rst>`_.

In addition you need the :gh:`qrcode <lincolnloop/python-qrcode>` Python
package installed. Get it via ``pip`` or your distribution package manager:

.. code:: sh

    pip3 install qrcode

The QR code is rendered using the :rst:`.. qr::` directive. Directive argument
is the data to render. The library will auto-scale the image based on the input
data size, you can override it using the optional :rst:`:size:` parameter.
Resulting SVG has the :css:`.m-image` class, meaning it's centered and at most
100% of page width.

.. code-figure::

    .. code:: rst

        .. qr:: https://mcss.mosra.cz/plugins/plots-and-graphs/#qr-code
            :size: 256px

    .. qr:: https://mcss.mosra.cz/plugins/plots-and-graphs/#qr-code
        :size: 256px
