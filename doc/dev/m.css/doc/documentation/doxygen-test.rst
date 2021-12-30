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

:save_as: documentation/doxygen/test/index.html
:breadcrumb:
    {filename}/documentation.rst Doc themes
    {filename}/documentation/doxygen.rst Doxygen
:css: {static}/static/m-dark.documentation.compiled.css

Lists
=====

Compound lists should have the same indentation as normal ones.

.. raw:: html

    <ul>
      <li>An item</li>
      <li>Another
        <ul>
          <li>Subitem</li>
        </ul>
      <li>Another</li>
    </ul>

    <ul class="m-doc">
      <li>An item</li>
      <li class="m-doc-expansible"><a href="#"></a>An expansible item. Verify that the indentation works. <span>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus ultrices a erat eu suscipit.</span>
        <ul class="m-doc">
          <li>Subitem</li>
          <li>Subitem</li>
          <li>Subitem</li>
          <li>Subitem</li>
        </ul>
      </li>
      <li class="m-doc-collapsible"><a href="#"></a>A collapsible item
        <ul class="m-doc">
          <li>Subitem</li>
        </ul>
      </li>
      <li>Another item. Verify that the indentation works. <span>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus ultrices a erat eu suscipit.</span></li>
    </ul>
