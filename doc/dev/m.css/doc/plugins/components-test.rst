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

:save_as: plugins/components/test/index.html
:breadcrumb: {filename}/plugins.rst Plugins
             {filename}/plugins/components.rst Components

Should match the rendering of
`CSS components test page <{filename}/css/components-test.rst>`_.

Blocks_
=======

.. container:: m-row

    .. container:: m-col-m-3 m-col-s-6

        .. block-default:: Default block

            Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus
            ultrices a erat eu suscipit. `Link. <#>`_

    .. container:: m-col-m-3 m-col-s-6

        .. block-primary:: Primary block

            Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus
            ultrices a erat eu suscipit. `Link. <#>`_

    .. container:: m-col-m-3 m-col-s-6

        .. block-success:: Success block

            Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus
            ultrices a erat eu suscipit. `Link. <#>`_

    .. container:: m-col-m-3 m-col-s-6

        .. block-warning:: Warning block

            Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus
            ultrices a erat eu suscipit. `Link. <#>`_

    .. container:: m-col-m-3 m-col-s-6

        .. block-danger:: Danger block

            Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus
            ultrices a erat eu suscipit. `Link. <#>`_

    .. container:: m-col-m-3 m-col-s-6

        .. block-info:: Info block

            Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus
            ultrices a erat eu suscipit. `Link. <#>`_

    .. container:: m-col-m-3 m-col-s-6

        .. block-dim:: Dim block

            Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus
            ultrices a erat eu suscipit. `Link. <#>`_

    .. container:: m-col-m-3 m-col-s-6

        .. block-flat:: Flat block

            Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus
            ultrices a erat eu suscipit. `Link. <#>`_

`Notes, frame`_
===============

.. container:: m-row

    .. container:: m-col-m-3 m-col-s-6

        .. note-default:: Default note

            Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus
            ultrices a erat eu suscipit. `Link. <#>`_

    .. container:: m-col-m-3 m-col-s-6

        .. note-primary:: Primary note

            Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus
            ultrices a erat eu suscipit. `Link. <#>`_

    .. container:: m-col-m-3 m-col-s-6

        .. note-success:: Success note

            Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus
            ultrices a erat eu suscipit. `Link. <#>`_

    .. container:: m-col-m-3 m-col-s-6

        .. note-warning:: Warning note

            Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus
            ultrices a erat eu suscipit. `Link. <#>`_

    .. container:: m-col-m-3 m-col-s-6

        .. note-danger:: Danger note

            Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus
            ultrices a erat eu suscipit. `Link. <#>`_

    .. container:: m-col-m-3 m-col-s-6

        .. note-info:: Info note

            Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus
            ultrices a erat eu suscipit. `Link. <#>`_

    .. container:: m-col-m-3 m-col-s-6

        .. note-dim:: Dim note

            Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus
            ultrices a erat eu suscipit. `Link. <#>`_

    .. container:: m-col-m-3 m-col-s-6

        .. frame:: Frame

            Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus
            ultrices a erat eu suscipit. `Link. <#>`_

Note w/o title, with applied class:

.. note-default::
    :class: m-text-center

    Some center-aligned content.

Block, with applied class:

.. block-warning:: Warning block
    :class: m-text-right

    Aligned to the right

Frame, w/o title, with applied class:

.. frame::
    :class: m-text-center

    Centered frame content

Flat code figure:

.. code-figure::
    :class: m-flat

    ::

        Some
            code
        snippet

    And a resulting output.

Console figure:

.. console-figure::

    .. include:: math-and-code-console.ansi
        :code: ansi

    And a description of that illegal crackery that's done above.
