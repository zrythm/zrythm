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

:slug: css/page-layout/test
:breadcrumb: {filename}/css.rst CSS
             {filename}/css/page-layout.rst Page layout

`Components in highlighted sections`_
=====================================

Click on the section header above to see the effect. Left border radius on all
elements should be flattened and nothing should jump when highlighted.

.. note-default::

    Default note

.. note-primary::

    Primary note

.. note-success::

    Success note

.. note-warning::

    Warning note

.. note-danger::

    Danger note

.. note-info::

    Info note

.. note-dim::

    Dim note

`Subsections`_
--------------

.. code:: c++

    int main() { }

.. block-default:: Default block

    Blocks don't change their appearance much.

.. block-flat:: Flat block

    Flat blocks don't change their appearance at all.

.. frame:: Frame

    Frame will have its left border fattened.

.. code-figure::

    .. code:: c++

        /* The code will have a custom border */

    The figure as well.

`Floating components`_
----------------------

.. container:: m-col-m-4 m-left-m

    .. note-success::

        Changed on the left side.

.. container:: m-col-m-4 m-right-m

    .. note-danger::

        No change if not fullsize.

Should behave as expected on the left side, but not on the right (unless the
window size is small). Centered note below should be marked also only when
touching the left side, not when centered.

.. container:: m-clearfix-l

    ..

.. container:: m-col-m-4 m-left-m

    .. block-success:: Block

        Changed on the left side.

.. container:: m-col-m-4 m-right-m

    .. block-danger:: Block

        No change if not fullsize.

Neither the blocks should do that.

.. container:: m-clearfix-l

    ..

.. container:: m-col-m-4 m-center-m

    .. block-danger:: Centered

        No change if not fullsize.

.. container:: m-col-m-4 m-center-m

    .. frame:: Watch the border

        No change if not fullsize.

.. container:: m-col-m-8 m-center-m

    .. code:: c++

        /* Change on the left on small screen sizes. */

.. container:: m-col-m-10 m-center-m

    .. console-figure::

        .. code:: shell-session

            $ code will have a custom border on small screens

        The figure as well.

`Floating components, inflated`_
--------------------------------

.. container:: m-col-l-4 m-left-l m-container-inflate

    .. note-success::

        Changed.

.. container:: m-col-l-4 m-right-l m-container-inflate

    .. note-success::

        Not changed if not on the left.

Lorem ipsum.

.. container:: m-clearfix-l

    ..

.. container:: m-container-inflate

    .. note-info:: Yes.

        This changes as well.

`Nested components`_
--------------------

Shouldn't be any difference.

.. container:: m-row

    .. container:: m-col-m-4 m-col-s-6

        .. note-default::

            Default note.

    .. container:: m-col-m-4 m-col-s-6

        .. block-primary:: Primary block

            Text.

    .. container:: m-col-m-4 m-col-s-6

        .. frame::

            A frame.

    .. container:: m-clearfix-m

        ..

    .. container:: m-col-m-4 m-col-s-6

        .. code:: hs

            :: -> :: -> ::

    .. container:: m-col-m-4 m-col-s-6

        .. class:: m-inverted
        .. code:: c++
            :hl_lines: 2

            int main() {
                return 666;
            }
