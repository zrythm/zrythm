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

Grid system
###########

:breadcrumb: {filename}/css.rst CSS
:footer:
    .. note-dim::
        :class: m-text-center

        `CSS <{filename}/css.rst>`_ | `Typography » <{filename}/css/typography.rst>`_

.. role:: css(code)
    :language: css
.. role:: html(code)
    :language: html

Inspired by `Bootstrap <https://getbootstrap.com>`_, the grid system is the
heart of a responsive website layout. The `m-grid.css <{filename}/css.rst>`_
file contains all the setup you need for it, it's completely standalone.
Besides that, in order to have devices recognize your website properly as
responsive and not zoom it out all the way to an unreadable mess, don't forget
to add this to your :html:`<head>`:

.. code:: html

    <meta name="viewport" content="width=device-width, initial-scale=1.0" />

.. contents::
    :class: m-block m-default

`Overview`_
===========

If you have never heard of it, it all boils down to a few breakpoints that
define how the website is wide on various screen sizes. For every one of them
you can define how your page and content layout will look like. In every case
you have 12 columns and it's completely up to you how you use them. Let's start
with an example.

.. code:: html

    <div class="m-container">
      <div class="m-row">
        <div class="m-col-t-6">.m-col-t-6</div>
        <div class="m-col-t-6">.m-col-t-6</div>
      </div>
      <div class="m-row">
        <div class="m-col-s-4 m-col-m-6">.m-col-s-4 .m-col-m-6</div>
        <div class="m-col-s-4 m-col-m-3">.m-col-s-4 .m-col-m-3</div>
        <div class="m-col-s-4 m-col-m-3">.m-col-s-4 .m-col-m-3</div>
      </div>
      <div class="m-row">
        <div class="m-col-l-7"><div class="m-frame m-center">.m-col-l-7</div></div>
        <div class="m-col-l-5"><div class="m-frame m-center">.m-col-l-5</div></div>
      </div>
    </div>

.. raw:: html

    <div class="m-row">
      <div class="m-col-t-6 m-nopad"><div class="m-frame m-text-center">.m-col-t-6</div></div>
      <div class="m-col-t-6 m-nopad"><div class="m-frame m-text-center">.m-col-t-6</div></div>
    </div>
    <div class="m-row">
      <div class="m-col-s-4 m-col-m-6 m-nopad"><div class="m-frame m-text-center">.m-col-s-4 .m-col-m-6</div></div>
      <div class="m-col-s-4 m-col-m-3 m-nopad"><div class="m-frame m-text-center">.m-col-s-4 .m-col-m-3</div></div>
      <div class="m-col-s-4 m-col-m-3 m-nopad"><div class="m-frame m-text-center">.m-col-s-4 .m-col-m-3</div></div>
    </div>
    <div class="m-row">
      <div class="m-col-l-7 m-nopady m-nopadx"><div class="m-frame m-text-center">.m-col-l-7</div></div>
      <div class="m-col-l-5 m-nopadt m-nopadx"><div class="m-frame m-text-center">.m-col-l-5</div></div>
    </div>

.. note-info::

    To see the effect, try to resize your browser window (or rotate the screen
    of your mobile device). Please note that the live examples here are not
    *exactly* matching the code --- I added a bunch of HTML elements and CSS
    classes to improve clarity.

In the above code, the outer :html:`<div class="m-container">` is taking care
of limiting the website layout width. On very narrow devices, the whole screen
width is used, on wider screens fixed sizes are used in order to make the
content layouting manageable. The :css:`.m-container` usually has :css:`.m-row`
items directly inside, but that's not a requirement --- you can put anything
there, if you just want to have some predictable width limitation to your
content.

The :html:`<div class="m-row">` denotes a row. The row is little more than
a delimiter for columns --- it just makes sure that two neighboring rows don't
interact with each other in a bad way.

The actual magic is done by the row elements denoted by :css:`.m-col-B-N`. The
``B`` is one of four breakpoints --- ``t`` for tiny, ``s`` for small, ``m`` for
medium and ``l`` for large --- based on screen width. The ``N`` is number of
columns that given element will span. So, looking at the code above,
:css:`.m-col-m-3` will span three columns out of twelve on medium-sized screen.

This setting is inherited upwards --- if you specify a column span for a
smaller screen width, it will get applied on larger width as well, unless you
override it with a setting for larger screen width. On the other hand, if given
screen width has no column width specified, the element will span the whole
row. So, again from above, the :css:`.m-col-s-4 .m-col-m-6` element will span
the whole row on tiny screen sizes, four columns on small screen sizes and six
columns on medium and large screen sizes.

`Detailed grid properties`_
===========================

.. class:: m-table

+-------------------+-----------------------+---------------------------+-------------------------------+
| Breakpoint        | Screen width range    | ``.m-container`` width    | Classes applied, ordered by   |
|                   | (inclusive)           |                           | priority                      |
+===================+=======================+===========================+===============================+
| ``t``, "tiny",    | less than 576px       | full screen width         | ``.m-col-t-*``                |
| portrait phones   |                       |                           |                               |
+-------------------+-----------------------+---------------------------+-------------------------------+
| ``s``, "small",   | 576px - 767px         | 560px                     | ``.m-col-s-*``,               |
| landscape phones  |                       |                           | ``.m-col-t-*``                |
+-------------------+-----------------------+---------------------------+-------------------------------+
| ``m``, "medium",  | 768px - 991px         | 750px                     | ``.m-col-m-*``,               |
| tablets, small    |                       |                           | ``.m-col-s-*``,               |
| desktops          |                       |                           | ``.m-col-t-*``                |
+-------------------+-----------------------+---------------------------+-------------------------------+
| ``l``, "large",   | 992px and up          | 960px                     | ``.m-col-l-*``,               |
| desktops, very    |                       |                           | ``.m-col-m-*``,               |
| large tablets     |                       |                           | ``.m-col-s-*``,               |
|                   |                       |                           | ``.m-col-t-*``                |
+-------------------+-----------------------+---------------------------+-------------------------------+

`Wrapping around`_
==================

Besides the above "all or nothing" scenario, where all the elements either form
a single row or are laid out one after another in separate rows, it's possible
to wrap the items around so they for example take four columns in a large
screen width and two rows of two columns in a small screen width. In such case
it's important to account for elements with different heights using a
:css:`.m-clearfix-*` for given breakpoint:

.. code:: html

    <div class="m-container">
      <div class="m-row">
        <div class="m-col-s-6 m-col-m-3">.m-col-s-6 .m-col-m-3<br/>...<br/>...</div>
        <div class="m-col-s-6 m-col-m-3">.m-col-s-6 .m-col-m-3</div>
        <div class="m-clearfix-s"></div>
        <div class="m-col-s-6 m-col-m-3">.m-col-s-6 .m-col-m-3<br/>...</div>
        <div class="m-col-s-6 m-col-m-3">.m-col-s-6 .m-col-m-3<br/>...</div>
      </div>
    </div>

.. raw:: html

    <div class="m-row">
      <div class="m-col-s-6 m-col-m-3 m-nopadx m-nopadt"><div class="m-frame m-text-center">.m-col-s-6 .m-col-m-3<br/>...<br/>...</div></div>
      <div class="m-col-s-6 m-col-m-3 m-nopadx m-nopadt"><div class="m-frame m-text-center">.m-col-s-6 .m-col-m-3</div></div>
      <div class="m-clearfix-s"></div>
      <div class="m-col-s-6 m-col-m-3 m-nopadx m-nopadt"><div class="m-frame m-text-center">.m-col-s-6 .m-col-m-3<br/>...</div></div>
      <div class="m-col-s-6 m-col-m-3 m-nopadx m-nopadt"><div class="m-frame m-text-center">.m-col-s-6 .m-col-m-3<br/>...</div></div>
    </div>

.. note-success::

    Shrink your browser window and then try to remove the
    :html:`<div class="m-clearfix-s">` element to see what it does in the above
    markup.

`Pushing and pulling`_
======================

It's possible to push and pull the elements around using :css:`.m-push-*` and
:css:`.m-pull-*` and even use this functionality to horizontally reorder
content based on screen width. Learn by example:

.. code:: html

    <div class="m-container">
      <div class="m-row">
        <div class="m-col-l-6 m-push-l-3">.m-col-l-6 .m-push-l-3</div>
      </div>
      <div class="m-row">
        <div class="m-col-s-8 m-push-s-4">.m-col-s-8 .m-push-s-4<br/>first</div>
        <div class="m-col-s-4 m-pull-s-8">.m-col-s-4 .m-pull-s-8<br/>second</div>
      </div>
    </div>

.. raw:: html

    <div class="m-row">
      <div class="m-col-l-6 m-push-l-3 m-nopad"><div class="m-frame m-text-center">.m-col-l-6 .m-push-l-3</div></div>
    </div>
    <div class="m-row">
      <div class="m-col-s-8 m-push-s-4 m-nopad"><div class="m-frame m-text-center">.m-col-s-8 .m-push-s-4<br/>first</div></div>
      <div class="m-col-s-4 m-pull-s-8 m-nopadx m-nopadt"><div class="m-frame m-text-center">.m-col-s-4 .m-pull-s-8<br/>second</div></div>
    </div>

.. note-warning::

    There may be some corner cases related to column span inheritance and
    pushing/pulling. If the output is not desired, try specifying the
    :css:`.m-push-*` and :css:`.m-pull-*` explicitly for all breakpoints.

`Floating around`_
==================

It's also possible to responsively float or align the elements around using
:css:`m-left-*`, :css:`m-right-*` and :css:`m-center-*` if you put the
:css:`.m-col-*` elements directly into text flow without wrapping them in a
:css:`.m-row` element. The following example will float the contents to the
right on medium-size screens, center them on small and put them full-width
on tiny screens:

.. code:: html

    <div class="m-col-s-6 m-center-s m-col-m-4 m-right-m">
      .m-col-s-6 .m-center-s .m-col-m-4 .m-right-m
    </div>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod
    tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam,
    quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo
    consequat.</p>

.. raw:: html

    <div class="m-col-s-6 m-center-s m-col-m-4 m-right-m">
    <div class="m-frame m-text-center">.m-col-s-6 .m-center-s .m-col-m-4 .m-right-m</div>
    </div>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod
    tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam,
    quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo
    consequat.</p>

.. container:: m-clearfix-m

    ..

Left-aligned floating blocks additionally have :css:`1rem` padding on the right
and right-aligned blocks on the left. All of them have implicitly padding on bottom, except if the element is a last child. Add a :css:`.m-nopadb` class to
them to disable bottom padding, if you want to preserve it even for a last
child, add an empty :html:`<div></div>` element after.

.. note-info::

    The floating works on any element, not just those that are :css:`.m-col-*`.
    Moreover, if you'll float `components <{filename}/css/components.rst>`_
    such as images or figures without using :css:`.m-col-*`, they will occupy
    exactly the width of their contents without being shrunk or scaled.

`Grid padding`_
===============

The :css:`.m-container` element pads its contents from left and right by
:css:`1rem`; the :css:`.m-row` adds a negative :css:`-1rem` margin on left and
right to reset it. Finally, the :css:`.m-col-*` elements have :css:`1rem`
padding on all sides to separate the contents. It's possible to override this
on the :css:`.m-container` and/or on :css:`.m-col-*` elements using
:css:`.m-nopad`, :css:`.m-nopadx`, :css:`m-nopady`, :css:`.m-nopadt`,
:css:`.m-nopadb`, :css:`.m-nopadl`, :css:`.m-nopadr` which remove all, just
horizontal or vertical padding, or padding on the top/bottom/left/right,
respectively.

`Grid nesting`_
===============

It's possible to nest the grid. Just place a :css:`.m-row` element inside an
:css:`.m-col-*` (it *doesn't* need to be a direct child) and put your
:css:`.m-col-*` elements inside. The inner grid will also have 12 columns, but
smaller ones.

.. note-info::

    Because the :css:`.m-container` element specifies a fixed width, it's not
    desirable to wrap the nested grid with it again.

`Removing the grid on larger screen sizes`_
===========================================

The usual behavior is to *add* the grid for larger screen sizes, but sometimes
you might want to do the opposite. It's possible to do that just by specifying
:css:`.m-col-*-12` for given breakpoint, but sometimes the CSS :css:`float`
property might cause problems. The :css:`.m-col-*-none` classes completely
remove all grid-related CSS styling for given screen size and up so the element
behaves like in its initial state.

.. code:: html

    <div class="m-container">
      <div class="m-row">
        <div class="m-col-s-8 m-col-l-none">.m-col-s-8 .m-col-l-none</div>
        <div class="m-col-s-4 m-col-l-none">.m-col-s-4 .m-col-l-none</div>
      </div>
    </div>

.. raw:: html

    <div class="m-row">
      <div class="m-col-s-8 m-col-l-none m-nopad"><div class="m-frame m-text-center">.m-col-s-8 .m-col-l-none</div></div>
      <div class="m-col-s-4 m-col-l-none m-nopad"><div class="m-frame m-text-center">.m-col-s-4 .m-col-l-none</div></div>
    </div>

`Hiding elements based on screen size`_
=======================================

The :css:`.m-show-*` and :css:`.m-hide-*` classes hide or show the element on
given screen size and up. Useful for example to show a different kind of
navigation on different devices.

.. code:: html

    <div class="m-show-m">.m-show-m<br/>shown on M and up</div>
    <div class="m-hide-l">.m-hide-l<br/>hidden on L and up</div>

.. raw:: html

    <div class="m-row">
        <div class="m-col-m-6 m-col-l-12 m-show-m m-nopad"><div class="m-frame m-text-center">.m-show-m<br/>shown on M and up</div></div>
        <div class="m-col-m-6 m-col-l-12 m-hide-l m-nopad"><div class="m-frame m-text-center">.m-hide-l<br/>hidden on L and up</div></div>
    </div>

`Inflatable nested grid`_
=========================

It's usual that the content area of the page doesn't span the full 12-column
width in order to avoid long unreadable lines. But sometimes one might want to
use the full available width --- for example to show big pictures or to fit
many things next to each other.

If you have a ten-column content area with one column space on each side, mark
your :css:`.m-container` element with :css:`.m-container-inflatable` and then
put your nested content in elements marked with :css:`.m-container-inflate`.

.. code:: html

    <div class="m-container m-container-inflatable">
      <div class="m-row">
        <div class="m-col-l-10 m-push-l-1">
          <div class="m-container-inflate">.m-container-inflate</div>
        </div>
      </div>
    </div>

.. raw:: html

    <div class="m-container-inflate">
      <div class="m-frame m-text-center">.m-container-inflate</div>
    </div>

The :css:`.m-container-inflate` block has implicitly a :css:`1rem` padding
after (but not on other sides and also not if it's the last child). Add a
:css:`.m-nopadb` class to it to disable that, if you want to preserve the
padding even for a last child, add an empty :html:`<div></div>` element after.

It's also possible to tuck floating content out of the page flow by combining
:css:`.m-left-*` with :css:`.m-container-inflate`:

.. code:: html

    <div class="m-container m-container-inflatable">
      <div class="m-row">
        <div class="m-col-l-10 m-push-l-1">
          <div class="m-container-inflate m-col-l-6 m-right-l">
            .m-container-inflate .m-col-l-6 .m-right-l
          </div>
          <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aenean id
          elit posuere, consectetur magna congue, sagittis est. Pellentesque
          est neque, aliquet nec consectetur in, mattis ac diam.</p>
        </div>
      </div>
    </div>

.. raw:: html

    <div class="m-container-inflate m-col-l-6 m-right-l">
      <div class="m-frame m-text-center">.m-container-inflate .m-col-l-6 m-right-l</div>
    </div>
    <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aenean id
    elit posuere, consectetur magna congue, sagittis est. Pellentesque
    est neque, aliquet nec consectetur in, mattis ac diam.</p>

.. note-warning::

    Similarly to `pushing and pulling`_, there may be corner cases where
    inflating of floating elements may not work. To fix that, try specifying
    all applicable :css:`.m-col-*-10` breakpoints for the inflatable column and
    :css:`.m-right-*` / :css:`.m-left-*` for the floating element explicitly.

`Debug CSS`_
============

It's sometimes hard to see why the layout isn't working as expected. Including
the `m-debug.css <{filename}/css.rst>`_ file will highlight the most usual
mistakes --- such as :css:`.m-row` not directly containing :css:`.m-col-*` or
two :css:`.m-container`\ s nested together --- with bright red background for
you to spot the problems better:

.. code:: html

    <link rel="stylesheet" href="m-dark.css" />

Other than highlighting problems, this file doesn't alter your website
appearance in any way. To save unnecessary requests and bandwidth, I recommend
that you remove the reference again when publishing the website.
