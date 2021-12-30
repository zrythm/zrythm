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

:save_as: css/grid/test/index.html
:breadcrumb: {filename}/css.rst CSS
             {filename}/css/grid.rst Grid system

See the `Components test <{filename}/css/components-test.rst#floating-around>`_
for a similar test but with components.

Floating around
===============

.. raw:: html

    <div class="m-col-m-5 m-right-m">
      <p>Should have no spacing on top/sides but on the bottom, extended to the
      sides on tiny. Lorem ipsum dolor.</p>
    </div>

    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aenean id
    elit posuere, consectetur magna congue, sagittis est. Pellentesque
    est neque, aliquet nec consectetur in, mattis ac diam. Aliquam
    placerat justo ut purus interdum, ac placerat lacus consequat. Ut dictum
    enim posuere metus porta, et aliquam ex condimentum. Proin sagittis nisi
    leo, ac pellentesque purus bibendum sit amet.</p>

    <div class="m-col-s-6 m-center-s">
      <p>Should have spacing on sides but not on the bottom, extended to the
      sides on tiny. Pellentesque est neque, aliquet nec consectetur.</p>
    </div>

Floating around, inflated
=========================

.. raw:: html

    <div class="m-container-inflate">
      <p>Should be extended to both sides, have padding on bottom an on sides
      the same as above, lines aligned. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aenean id elit posuere, consectetur magna congue,
      sagittis est. Pellentesque est neque, aliquet nec consectetur in, mattis
      ac diam. Aliquam placerat justo ut purus interdum, ac placerat lacus
      consequat.</p>
    </div>

    <div class="m-container-inflate m-col-l-4 m-right-l">
      <p>Should be extended to the right, have padding on bottom an on the
      right side the same as above, lines aligned. Lorem ipsum dolor.</p>
    </div>

    <div class="m-container-inflate m-col-l-4 m-left-l">
      <p>Should be extended to the left, have padding on bottom an on the
      outside the same as above, lines aligned. Lorem ipsum dolor.</p>
    </div>

    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aenean id
    elit posuere, consectetur magna congue, sagittis est. Pellentesque
    est neque, aliquet nec consectetur in, mattis ac diam. Aliquam placerat
    justo ut purus interdum, ac placerat lacus consequat. Ut dictum enim
    posuere metus porta, et aliquam ex condimentum. Proin sagittis nisi leo, ac
    pellentesque purus bibendum sit amet. Aliquam placerat justo ut purus
    interdum, ac placerat lacus consequat. Ut dictum enim posuere metus porta,
    et aliquam ex condimentum. </p>
