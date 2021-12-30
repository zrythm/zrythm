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

Components
##########

:breadcrumb: {filename}/plugins.rst Plugins
:footer:
    .. note-dim::
        :class: m-text-center

        `« HTML sanity <{filename}/plugins/htmlsanity.rst>`_ | `Plugins <{filename}/plugins.rst>`_ | `Images » <{filename}/plugins/images.rst>`_

.. role:: html(code)
    :language: html
.. role:: py(code)
    :language: py
.. role:: rst(code)
    :language: rst

Most of m.css `CSS components <{filename}/css/components.rst>`_ are exposed to
Pelican via custom :abbr:`reST <reStructuredText>` directives. Unlike with pure
CSS, the directives are *not* prefixed with ``m-`` to save some typing ---
which is the most important thing when authoring content.

.. contents::
    :class: m-block m-default

`How to use`_
=============

`Pelican`_
----------

Download the `m/components.py <{filename}/plugins.rst>`_ file, put it
including the ``m/`` directory into one of your :py:`PLUGIN_PATHS` and add
:py:`m.components` package to your :py:`PLUGINS` in ``pelicanconf.py``. This
plugin assumes presence of `m.htmlsanity <{filename}/plugins/htmlsanity.rst>`_.

.. code:: python

    PLUGINS += ['m.htmlsanity', 'm.components']

`Python doc theme`_
-------------------

Simply list the plugin in your :py:`PLUGINS`. The `m.htmlsanity`_ plugin is
available always, no need to mention it explicitly:

.. code:: py

    PLUGINS += ['m.components']

`Doxygen theme`_
----------------

Unfortunately, due to a lack of extensibility of the Doxygen markup language,
it's not possible to provide the components through easy-to-use commands. All
you have is the ability to apply CSS classes using ``@m_class``, ``@m_span``
and ``@m_div``. See the
`Doxygen theme-specific commands <http://localhost:8000/documentation/doxygen/#theme-specific-commands>`_
for more information.

`Transitions`_
==============

Use :rst:`.. transition::` directive to create a `transition <{filename}/css/typography.rst#transitions>`_:

.. code-figure::

    .. code:: rst

        Aenean tellus turpis, suscipit quis iaculis ut, suscipit nec magna. Vestibulum
        finibus sit amet neque nec volutpat. Suspendisse sit amet nisl in orci posuere
        mattis.

        .. transition:: ~ ~ ~

        Praesent eu metus sed felis faucibus placerat ut eu quam. Aliquam convallis
        accumsan ante sit amet iaculis. Phasellus rhoncus hendrerit leo vitae lacinia.
        Maecenas iaculis dui ex, eu interdum lacus ornare sit amet.

    Aenean tellus turpis, suscipit quis iaculis ut, suscipit nec magna.
    Vestibulum finibus sit amet neque nec volutpat. Suspendisse sit amet nisl
    in orci posuere mattis.

    .. transition:: ~ ~ ~

    Praesent eu metus sed felis faucibus placerat ut eu quam. Aliquam convallis
    accumsan ante sit amet iaculis. Phasellus rhoncus hendrerit leo vitae
    lacinia. Maecenas iaculis dui ex, eu interdum lacus ornare sit amet.

`Blocks, notes, frame`_
=======================

Use :rst:`.. block-default::`, :rst:`.. block-primary::` etc. directives to create
`blocks <{filename}/css/components.rst#blocks>`_; use :rst:`.. note-default::`,
:rst:`.. note-primary::` etc. or :rst:`.. frame::` directives to create
`notes and frames <{filename}/css/components.rst#notes-frame>`_. Blocks require
title to be present, notes and frames have it optional. Internally, these
elements are represented as a
`topic directive <http://docutils.sourceforge.net/docs/ref/rst/directives.html#topic>`_.
Use the :rst:`:class:` option to specify additional CSS classes.

.. code-figure::

    .. code:: rst

        .. block-danger:: Danger block

            Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus ultrices a
            erat eu suscipit. `Link. <#>`_

        .. note-success:: Success note

            Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus ultrices a
            erat eu suscipit. `Link. <#>`_

        .. frame:: Frame

            Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus ultrices a
            erat eu suscipit. `Link. <#>`_

    .. container:: m-row

        .. container:: m-col-m-4

            .. block-danger:: Danger block

                Lorem ipsum dolor sit amet, consectetur adipiscing elit.
                Vivamus ultrices a erat eu suscipit. `Link. <#>`_

        .. container:: m-col-m-4

            .. note-success:: Success note

                Lorem ipsum dolor sit amet, consectetur adipiscing elit.
                Vivamus ultrices a erat eu suscipit. `Link. <#>`_

        .. container:: m-col-m-4

            .. frame:: Frame

                Lorem ipsum dolor sit amet, consectetur adipiscing elit.
                Vivamus ultrices a erat eu suscipit. `Link. <#>`_

`Code, math and graph figure`_
==============================

Use :rst:`.. code-figure::` to denote a `code figure <{filename}/css/components.rst#code-figure>`_.
Then put a literal code block denoted by :rst:`::` or a :rst:`.. code::`
directive as the first element inside. Use the :rst:`:class:` option to specify
additional CSS classes. The optional directive parameter can be used for a
figure caption.

.. code-figure::

    .. code:: rst

        .. code-figure::

            ::

                Some
                    sample
                code

            And a resulting output.

    .. code-figure::

        ::

            Some
                sample
            code

        And a resulting output.

Use :rst:`.. console-figure::` to denote code figure styled for a
`console listing <{filename}/css/components.rst#colored-terminal-output>`_.
Similarly, :rst:`.. math-figure::` denotes a `math figure <{filename}/css/components.rst#math-figure>`_
and :rst:`.. graph-figure::` denotes a `graph figure <{filename}/css/components.rst#graph-figure>`_.

`Text`_
=======

Use :rst:`.. text-default::`, :rst:`.. text-primary::` etc. directives to
`color a block of text <{filename}/css/components.rst#text>`_. Internally,
these elements are represented as a `container directive <http://docutils.sourceforge.net/docs/ref/rst/directives.html#container>`_.
Use the :rst:`:class:` option to specify additional CSS classes.

.. code-figure::

    .. code:: rst

        .. text-info::
            :class: m-text-center

            Info text. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus
            ultrices a erat eu suscipit. Aliquam pharetra imperdiet tortor sed
            vehicula. `Link. <#>`_

    .. text-info::
        :class: m-text-center

        Info text. Lorem ipsum dolor sit amet, consectetur adipiscing elit.
        Vivamus ultrices a erat eu suscipit. Aliquam pharetra imperdiet tortor
        sed vehicula. `Link. <#>`_

There are no interpreted text roles provided for inline colored text, but you
can define a custom one and add the CSS classes to it, potentially deriving it
from either the :rst:`:emphasis:` or :rst:`:strong:` role to combine color with
emphasis or strong text:

.. code-figure::

    .. code:: rst

        .. role:: text-dim
            :class: m-text m-dim

        .. role:: text-warning-strong(strong)
            :class: m-text m-warning

        Aenean id elit posuere, consectetur magna congue, sagittis est.
        :text-dim:`Dim inline text.` Pellentesque est neque, aliquet nec consectetur
        in, mattis ac diam. :text-warning-strong:`Warning strong text.`

    .. role:: text-dim
        :class: m-text m-dim

    .. role:: text-warning-strong(strong)
        :class: m-text m-warning

    Aenean id elit posuere, consectetur magna congue, sagittis est.
    :text-dim:`Dim inline text.` Pellentesque est neque, aliquet nec
    consectetur in, mattis ac diam. :text-warning-strong:`Warning strong text.`

`Button links`_
===============

Use :rst:`.. button-default::`, :rst:`.. button-primary::` etc. or
:rst:`.. button-flat::` directives to create a
`button link <{filename}/css/components.rst#button-links>`_. Directive argument
is the URL, directive contents are button title. Use the :rst:`:class:` option
to specify additional CSS classes. Use two paragraphs of content to create a
button with title and description.

.. code-figure::

    .. code:: rst

            .. button-danger:: #

                Order now!

            .. button-primary:: #

                Download the thing

                Any platform, 5 kB.

    .. container:: m-text-center

        .. button-danger:: #

            Order now!

        .. container:: m-clearfix-t

            ..

        .. button-primary:: #

            Download the thing

            Any platform, 5 kB.

`Tables`_
=========

Mark your reST table with :rst:`..class:: m-table` to give it styling.

.. code-figure::

    .. code:: rst

        .. class:: m-table

        = ============= ================
        # Heading       Second heading
        = ============= ================
        1 Cell          Second cell
        2 2nd row cell  2nd row 2nd cell
        = ============= ================

    .. class:: m-table m-center-t

    = ============= ================
    # Heading       Second heading
    = ============= ================
    1 Cell          Second cell
    2 2nd row cell  2nd row 2nd cell
    = ============= ================

.. todo: cell coloring, footer etc.

`Labels`_
=========

Use :rst:`:label-default:` etc. or :rst:`:label-flat-default:` etc. to create
inline `text labels <{filename}/css/components.rst#labels>`_.

.. code-figure::

    .. code:: rst

        -   Design direction and project goals :label-success:`done`
        -   Automated testing :label-danger:`missing`
            :label-flat-warning:`in progress`

    -   Design direction and project goals :label-success:`done`
    -   Automated testing :label-danger:`missing`
        :label-flat-warning:`in progress`

`Other m.css features`_
=======================

You can use :rst:`.. container::` directive to add a wrapping :html:`<div>`
around most elements. Parameters of the directive are CSS classes. For example,
arranging content in three-column grid can be done like this:

.. code-figure::

    .. code:: rst

        .. container:: m-row

            .. container:: m-col-m-4

                Left column content.

            .. container:: m-col-m-4

                Middle column content.

            .. container:: m-col-m-4

                Right column content.

    .. container:: m-row

        .. container:: m-col-m-4 m-text-center

            Left column content.

        .. container:: m-col-m-4 m-text-center

            Middle column content.

        .. container:: m-col-m-4 m-text-center

            Right column content.

For inline components not mentioned above, derive a custom role with additional
CSS classes. For example:

.. code-figure::

    .. code:: rst

        .. role:: text-em-strong
            :class: m-text m-em m-strong

        You *should* be **very** aware of the :text-em-strong:`potential danger`.

    .. role:: text-em-strong
        :class: m-text m-em m-strong

    You *should* be **very** aware of the :text-em-strong:`potential danger`.
