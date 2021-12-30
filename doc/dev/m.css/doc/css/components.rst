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

:breadcrumb: {filename}/css.rst CSS
:footer:
    .. note-dim::
        :class: m-text-center

        `« Typography <{filename}/css/typography.rst>`_ | `CSS <{filename}/css.rst>`_ | `Page layout » <{filename}/css/page-layout.rst>`_

.. role:: css(code)
    :language: css
.. role:: html(code)
    :language: html
.. role:: rst(code)
    :language: rst

In `m-components.css <{filename}/css.rst>`_, m.css provides a set of basic
components for further improving the layout and display of authored content.

.. contents::
    :class: m-block m-default

`Colors`_
=========

Similarly to Bootstrap, m.css has a set of predefined colors that affect how a
certain component looks. This works on majority of components shown on this
page. The colors are:

-   :css:`.m-default` is a default style that doesn't grab attention
-   :css:`.m-primary` is meant to be used to highlight the primary element on a
    page
-   :css:`.m-success` is good to highlight happy things
-   :css:`.m-warning` catches attention, but doesn't hurt
-   :css:`.m-danger` brings really bad news
-   :css:`.m-info` is for side notes
-   :css:`.m-dim` is shy and doesn't want you to be bothered by the information
    it brings

`Blocks`_
=========

Blocks, defined by :css:`.m-block`, wrap the content in a light frame and add a
bolder colored bar on the left. Use in combination with one of the color styles
above. Block caption should go into :html:`<h3>` (or :html:`<h4>`,
:html:`<h5>`, :html:`<h6>`) and is colored in respect to the color style as
well. Text and links inside the text always have the default color, except for
:css:`.m-block.m-dim`. It's also possible to have a block without the border,
just add :css:`.m-flat` class to it.

It's recommended to use the :html:`<aside>` element to highlight the semantics,
but the CSS class can be used on any block element.

.. code-figure::

    .. code:: html

        <aside class="m-block m-default">
          <h3>Default block</h3>
          Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus ultrices
          a erat eu suscipit. <a href="#">Link.</a>
        </aside>

    .. raw:: html

        <div class="m-row">
          <div class="m-col-m-3 m-col-s-6">
            <aside class="m-block m-default">
              <h3>Default block</h3>
              Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus ultrices a erat eu suscipit. <a href="#">Link.</a>
            </aside>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <aside class="m-block m-primary">
              <h3>Primary block</h3>
              Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus ultrices a erat eu suscipit. <a href="#">Link.</a>
            </aside>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <aside class="m-block m-success">
              <h3>Success block</h3>
              Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus ultrices a erat eu suscipit. <a href="#">Link.</a>
            </aside>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <aside class="m-block m-warning">
              <h3>Warning block</h3>
              Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus ultrices a erat eu suscipit. <a href="#">Link.</a>
            </aside>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <aside class="m-block m-danger">
              <h3>Danger block</h3>
              Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus ultrices a erat eu suscipit. <a href="#">Link.</a>
            </aside>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <aside class="m-block m-info">
              <h3>Info block</h3>
              Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus ultrices a erat eu suscipit. <a href="#">Link.</a>
            </aside>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <aside class="m-block m-dim">
              <h3>Dim block</h3>
              Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus ultrices a erat eu suscipit. <a href="#">Link.</a>
            </aside>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <aside class="m-block m-flat">
              <h3>Flat block</h3>
              Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus ultrices a erat eu suscipit. <a href="#">Link.</a>
            </aside>
          </div>
        </div>

.. note-info::

    The `Pelican Components plugin <{filename}/plugins/components.rst#blocks-notes-frame>`__
    is able to produce blocks conveniently using :rst:`.. block-::` directives
    in your :abbr:`reST <reStructuredText>` markup.

`Badges`_
=========

Badges are blocks together with an avatar, containing for example info about
the author of given article. Simply add :css:`.m-badge` to your colored block
element and put an :html:`<img>` element with the avatar as the first child.
Only :html:`<h3>` is supported for a badge. For standalone rounded avatars, see
`Images`_ below.

.. code-figure::

    .. code:: html

        <div class="m-block m-badge m-success">
          <img src="author.jpg" alt="The Author" />
          <h3>About the author</h3>
          <p><a href="#">The Author</a> is ...</p>
        </div>

    .. raw:: html

        <div class="m-block m-badge m-success">
          <img src="{static}/static/mosra.jpg" alt="The Author" />
          <h3>About the author</h3>
          <p><a href="http://blog.mosra.cz">The Author</a> is not really
          smiling at you from this avatar. Sorry about that. He knows that and
          he promises to improve. Stalk him
          <a href="https://twitter.com/czmosra">on Twitter</a> if you want to
          get notified when he gets a better avatar.</p>
        </div>

.. note-info::

    The `Pelican Metadata plugin <{filename}/plugins/metadata.rst>`_  is able
    to automatically render author and category badges for articles.

`Notes, frame`_
===============

Unlike blocks, notes are meant to wrap smaller bits of information. Use the
:css:`.m-note` CSS class together with desired color class. A note is also
slightly rounded and has everything colored, the background, the caption, text
and also links. The :html:`<h3>` (:html:`<h4>`, :html:`<h5>`, :html:`<h6>`)
caption tag is optional.

Besides notes, there is a frame element defined by :css:`.m-frame`, which just
wraps your content in a slightly rounded border. No color classes apply to a
frame.

Like with blocks, tt's recommended to use the :html:`<aside>` element for
semantic purposes, but the CSS classes can be used on any block element.

.. code-figure::

    .. code:: html

        <aside class="m-note m-success">
          <h3>Success note</h3>
          Lorem ipsum dolor sit amet, consectetur adipiscing elit. <a href="#">Link.</a>
        </aside>

    .. raw:: html

        <div class="m-row">
          <div class="m-col-m-3 m-col-s-6">
            <aside class="m-note m-default">
              <h3>Default note</h3>
              Lorem ipsum dolor sit amet, consectetur adipiscing elit. <a href="#">Link.</a>
            </aside>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <aside class="m-note m-primary">
              <h3>Primary note</h3>
              Lorem ipsum dolor sit amet, consectetur adipiscing elit. <a href="#">Link.</a>
            </aside>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <aside class="m-note m-success">
              <h3>Success note</h3>
              Lorem ipsum dolor sit amet, consectetur adipiscing elit. <a href="#">Link.</a>
            </aside>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <aside class="m-note m-warning">
              <h3>Warning note</h3>
              Lorem ipsum dolor sit amet, consectetur adipiscing elit. <a href="#">Link.</a>
            </aside>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <aside class="m-note m-danger">
              <h3>Danger note</h3>
              Lorem ipsum dolor sit amet, consectetur adipiscing elit. <a href="#">Link.</a>
            </aside>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <aside class="m-note m-info">
              <h3>Info note</h3>
              Lorem ipsum dolor sit amet, consectetur adipiscing elit. <a href="#">Link.</a>
            </aside>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <aside class="m-note m-dim">
              <h3>Dim note</h3>
              Lorem ipsum dolor sit amet, consectetur adipiscing elit. <a href="#">Link.</a>
            </aside>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <aside class="m-frame">
              <h3>Frame</h3>
              Lorem ipsum dolor sit amet, consectetur adipiscing elit. <a href="#">Link.</a>
            </aside>
          </div>
        </div>

.. note-info::

    Notes and frames can be created conveniently using :rst:`.. note-::` and
    :rst:`.. frame::` directives in your :abbr:`reST <reStructuredText>` markup
    using the `Pelican Components plugin <{filename}/plugins/components.rst#blocks-notes-frame>`__.

`Text`_
=======

Use :css:`.m-text` CSS class together with desired color class to color a
paragraph or inline text.

.. code-figure::

    .. code:: html

        <p class="m-text m-warning">Warning text. Lorem ipsum dolor sit amet,
        consectetur adipiscing elit. Vivamus ultrices a erat eu suscipit. Aliquam
        pharetra imperdiet tortor sed vehicula. <a href="#">Link.</a></p>

    .. raw:: html

        <div class="m-row">
          <div class="m-col-m-3 m-col-s-6">
            <p class="m-text m-default m-noindent">Default text. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus ultrices a erat eu suscipit. Aliquam pharetra imperdiet tortor sed vehicula. <a href="#">Link.</a></p>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <p class="m-text m-primary m-noindent">Primary text. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus ultrices a erat eu suscipit. Aliquam pharetra imperdiet tortor sed vehicula. <a href="#">Link.</a></p>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <p class="m-text m-success m-noindent">Success text. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus ultrices a erat eu suscipit. Aliquam pharetra imperdiet tortor sed vehicula. <a href="#">Link.</a></p>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <p class="m-text m-warning m-noindent">Warning text. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus ultrices a erat eu suscipit. Aliquam pharetra imperdiet tortor sed vehicula. <a href="#">Link.</a></p>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <p class="m-text m-danger m-noindent">Danger text. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus ultrices a erat eu suscipit. Aliquam pharetra imperdiet tortor sed vehicula. <a href="#">Link.</a></p>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <p class="m-text m-info m-noindent">Info text. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus ultrices a erat eu suscipit. Aliquam pharetra imperdiet tortor sed vehicula. <a href="#">Link.</a></p>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <p class="m-text m-dim m-noindent">Dim text. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus ultrices a erat eu suscipit. Aliquam pharetra imperdiet tortor sed vehicula. <a href="#">Link.</a></p>
          </div>
        </div>

.. note-info::

    Colored text paragraphs can be conveniently created using :rst:`.. text-::`
    directives in your :abbr:`reST <reStructuredText>` markup using the
    `Pelican Components plugin <{filename}/plugins/components.rst#text>`__.

Apply :css:`.m-tiny`, :css:`.m-small` or :css:`.m-big` CSS class together with
:css:`.m-text` to make the text appear smaller or larger.

.. code-figure::

    .. code:: html

        <p class="m-text m-big">Larger text. Lorem ipsum dolor sit amet, consectetur
        adipiscing elit. Vivamus ultrices a erat eu suscipit. Aliquam pharetra
        imperdiet tortor sed vehicula.</p>
        <p class="m-text m-small">Smaller text. Lorem ipsum dolor sit amet, consectetur
        adipiscing elit. Vivamus ultrices a erat eu suscipit. Aliquam pharetra
        imperdiet tortor sed vehicula.</p>
        <p class="m-text m-tiny">Tiny text. Lorem ipsum dolor sit amet, consectetur
        adipiscing elit. Vivamus ultrices a erat eu suscipit. Aliquam pharetra
        imperdiet tortor sed vehicula.</p>

    .. raw:: html

        <p class="m-text m-big">Larger text. Lorem ipsum dolor sit amet, consectetur
        adipiscing elit. Vivamus ultrices a erat eu suscipit. Aliquam pharetra
        imperdiet tortor sed vehicula.</p>
        <p class="m-text m-small">Smaller text. Lorem ipsum dolor sit amet, consectetur
        adipiscing elit. Vivamus ultrices a erat eu suscipit. Aliquam pharetra
        imperdiet tortor sed vehicula.</p>
        <p class="m-text m-tiny">Tiny text. Lorem ipsum dolor sit amet, consectetur
        adipiscing elit. Vivamus ultrices a erat eu suscipit. Aliquam pharetra
        imperdiet tortor sed vehicula.</p>

`Button links`_
===============

To highlight important links such as file download, you can style them as
buttons. Use :css:`.m-button` CSS class together with desired color class on an
:html:`<a>` tag. Use :css:`.m-flat` instead of a color class to make the button
flat. It is then styled similarly to a link, but with bigger padding around.
The button is by default centered, apply a :css:`.m-fullwidth` class to it to
display it as a full-width block with center-aligned label.

.. code-figure::

    .. code:: html

        <div class="m-button m-success m-fullwidth"><a href="#">Success button</a></div>

    .. raw:: html

        <div class="m-row">
          <div class="m-col-m-3 m-col-s-6">
            <div class="m-button m-default m-fullwidth"><a href="#">Default button</a></div>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <div class="m-button m-primary m-fullwidth"><a href="#">Primary button</a></div>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <div class="m-button m-success m-fullwidth"><a href="#">Success button</a></div>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <div class="m-button m-warning m-fullwidth"><a href="#">Warning button</a></div>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <div class="m-button m-danger m-fullwidth"><a href="#">Danger button</a></div>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <div class="m-button m-info m-fullwidth"><a href="#">Info button</a></div>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <div class="m-button m-dim m-fullwidth"><a href="#">Dim button</a></div>
          </div>
          <div class="m-col-m-3 m-col-s-6">
            <div class="m-button m-flat m-fullwidth"><a href="#">Flat button</a></div>
          </div>
        </div>

You can put two :html:`<div>`\ s with :css:`.m-big` and :css:`.m-small` CSS
class inside the :html:`<a>` to achieve the following effect:

.. code-figure::

    .. code:: html

        <div class="m-button m-primary">
          <a href="#">
            <div class="m-big">Download the thing</div>
            <div class="m-small">Any platform, 5 kB.</div>
          </a>
        </div>

    .. raw:: html

        <div class="m-button m-primary">
          <a href="#">
            <div class="m-big">Download the thing</div>
            <div class="m-small">Any platform, 5 kB.</div>
          </a>
        </div>

.. note-info::

    Buttons can be conveniently created using :rst:`.. button-::` directives in
    your :abbr:`reST <reStructuredText>` markup using the
    `Pelican Components plugin <{filename}/plugins/components.rst#button-links>`__.

`Labels`_
=========

Use :css:`.m-label` together with a color class to style a label. Combine with
:css:`.m-flat` to create a less noticeable label. The label size adapts to size
of surrounding text.

.. code-figure::

    .. code:: html

        <h3>An article <span class="m-label m-success">updated</span></h3>
        <p class="m-text m-dim">That's how things are now.
        <span class="m-label m-flat m-primary">clarified</span></p>

    .. raw:: html

        <h3>An article <span class="m-label m-success">updated</span></h3>
        <p class="m-text m-dim">That's how things are now.
        <span class="m-label m-flat m-primary">clarified</span></p>

.. note-info::

    The `Pelican Components plugin <{filename}/plugins/components.rst#labels>`__
    provides convenience :rst:`:label-:` and :rst:`:label-flat-:` interpreted
    text roles for labels as well.

`Tables`_
=========

Use :css:`.m-table` to apply styling to a table. The table is centered by
default; rows are separated by lines, with :html:`<thead>` and :html:`<tfoot>`
being separated by a thicker line. The :html:`<th>` element is rendered in
bold, all :html:`<th>` and :html:`<td>` are aligned to left while table
:html:`<caption>` is centered. Example table:

.. code-figure::

    .. code:: html

        <table class="m-table">
          <caption>Table caption</caption>
          <thead>
            <tr>
              <th>#</th>
              <th>Heading</th>
              <th>Second<br/>heading</th>
            </tr>
          </thead>
          <tbody>
            <tr>
              <th scope="row">1</th>
              <td>Cell</td>
              <td>Second cell</td>
            </tr>
          </tbody>
          <tbody>
            <tr>
              <th scope="row">2</th>
              <td>2nd row cell</td>
              <td>2nd row 2nd cell</td>
            </tr>
          </tbody>
          <tfoot>
            <tr>
              <th>&Sigma;</th>
              <td>Footer</td>
              <td>Second<br/>footer</td>
            </tr>
          </tfoot>
        </table>

    .. raw:: html

        <table class="m-table">
          <caption>Table caption</caption>
          <thead>
            <tr>
              <th>#</th>
              <th>Heading</th>
              <th>Second<br/>heading</th>
            </tr>
          </thead>
          <tbody>
            <tr>
              <th scope="row">1</th>
              <td>Cell</td>
              <td>Second cell</td>
            </tr>
            <tr>
              <th scope="row">2</th>
              <td>2nd row cell</td>
              <td>2nd row 2nd cell</td>
            </tr>
          </tbody>
          <tfoot>
            <tr>
              <th>&Sigma;</th>
              <td>Footer</td>
              <td>Second<br/>footer</td>
            </tr>
          </tfoot>
        </table>

Rows are highlighted on hover, if you want to disable that, put :css:`.m-flat`
CSS class on the :html:`<table>` element. You can also put :css:`.m-thin` onto
:html:`<th>` elements to remove the bold styling. Similarly to other
components, you can color particular :html:`<tr>` or :html:`<td>` elements
using the color classes from above:

.. raw:: html

    <div class="m-scroll"><table class="m-table m-fullwidth">
      <tbody>
        <tr class="m-default">
          <td>Default row</td>
          <td>Lorem</td>
          <td>ipsum</td>
          <td>dolor</td>
          <td>sit</td>
          <td>amet</td>
          <td><a href="#">Link</a></td>
        </tr>
        <tr class="m-primary">
          <td>Primary row</td>
          <td>Lorem</td>
          <td>ipsum</td>
          <td>dolor</td>
          <td>sit</td>
          <td>amet</td>
          <td><a href="#">Link</a></td>
        </tr>
        <tr class="m-success">
          <td>Success row</td>
          <td>Lorem</td>
          <td>ipsum</td>
          <td>dolor</td>
          <td>sit</td>
          <td>amet</td>
          <td><a href="#">Link</a></td>
        </tr>
        <tr class="m-warning">
          <td>Warning row</td>
          <td>Lorem</td>
          <td>ipsum</td>
          <td>dolor</td>
          <td>sit</td>
          <td>amet</td>
          <td><a href="#">Link</a></td>
        </tr>
        <tr class="m-danger">
          <td>Danger row</td>
          <td>Lorem</td>
          <td>ipsum</td>
          <td>dolor</td>
          <td>sit</td>
          <td>amet</td>
          <td><a href="#">Link</a></td>
        </tr>
        <tr class="m-info">
          <td>Info row</td>
          <td>Lorem</td>
          <td>ipsum</td>
          <td>dolor</td>
          <td>sit</td>
          <td>amet</td>
          <td><a href="#">Link</a></td>
        </tr>
        <tr class="m-dim">
          <td>Dim row</td>
          <td>Lorem</td>
          <td>ipsum</td>
          <td>dolor</td>
          <td>sit</td>
          <td>amet</td>
          <td><a href="#">Link</a></td>
        </tr>
      </tbody>
    </table></div>

Mark the table with :html:`.m-big` to inflate it with more spacing, for example
when designing a high-level product category overview.

Similarly to `lists <{filename}/css/typography.rst#lists-diaries>`_, if using
:html:`<p>` elements inside :html:`<td>`, they are neither indented nor
justified.

`Images`_
=========

Putting :css:`.m-image` class onto an :html:`<img>` / :html:`<svg>` tag makes
the image centered, slightly rounded and sets its max width to 100%. Adding
:css:`.m-fullwidth` on the image element works as expected. For accessibility
reasons it's a good practice to include the ``alt`` attribute.

.. code-figure::

    .. code:: html

        <img src="flowers.jpg" alt="Flowers" class="m-image" />

    .. raw:: html

        <img src="{static}/static/flowers-small.jpg" alt="Flowers" class="m-image" />

To make the image clickable, wrap the :html:`<a>` tag in an additional
:html:`<div>` and put the :css:`.m-image` class on the :html:`<div>` element
instead of on the image. This will ensure that only the image is clickable and
not the surrounding area:

.. code-figure::

    .. code:: html

        <div class="m-image">
          <a href="flowers.jpg"><img src="flowers.jpg" /></a>
        </div>

    .. raw:: html

        <div class="m-image">
          <a href="{static}/static/flowers.jpg"><img src="{static}/static/flowers-small.jpg" /></a>
        </div>

.. note-info::

    Images can be conveniently created with an :rst:`.. image::` directive in
    your :abbr:`reST <reStructuredText>` markup using the
    `Pelican Images plugin <{filename}/plugins/images.rst>`_.

For avatars, similarly to the `Badges`_ above, applying a :css:`.m-badge` class
together with :css:`.m-image` will make the image round. Works for both plain
:html:`<img>` and clickable images wrapped in :html:`<div class="m-image">`.

.. code-figure::

    .. code:: html

        <img src="author.jpg" alt="The Author" class="m-image m-badge" />

    .. raw:: html

        <img src="{static}/static/mosra.jpg" alt="The Author" class="m-image m-badge" style="width: 125px" />

`Figures`_
==========

Use the HTML5 :html:`<figure>` tag together with :css:`.m-figure` to style it.
As with plain image, it's by default centered, slightly rounded and has a
border around the caption and description. The caption is expected to be in the
:html:`<figcaption>` element. The description is optional, but should be
wrapped in some tag as well (for example a :html:`<span>`). The
:css:`.m-fullwidth` class works here too and you can also wrap the
:html:`<img>` / :html:`<svg>` element in an :html:`<a>` tag to make it
clickable.

Figure always expects at least the caption to be present. If you want just an
image, use the plain image tag. If you have a lot of figures on the page and
the border is distracting, apply the :css:`.m-flat` class to hide it.
Optionally you can color the figure border and caption by adding one of the
`CSS color classes <#colors>`_ to the :html:`<figure>` element.

.. code-figure::

    .. code:: html

        <figure class="m-figure">
          <img src="ship.jpg" alt="Ship" />
          <figcaption>A Ship</figcaption>
          <span>Photo © <a href="http://blog.mosra.cz/">The Author</a></span>
        </figure>

    .. raw:: html

        <figure class="m-figure">
          <img src="{static}/static/ship-small.jpg" alt="Ship" />
          <figcaption>A Ship</figcaption>
          <span>Photo © <a href="http://blog.mosra.cz/">The Author</a></span>
        </figure>

.. note-info::

    Figures can be conveniently created with a :rst:`.. figure::` directive in
    your :abbr:`reST <reStructuredText>` markup using the
    `Pelican Images plugin <{filename}/plugins/images.rst>`_.

`Image grid`_
=============

Inspired by `image grids on Medium <https://blog.medium.com/introducing-image-grids-c592e5bc16d8>`_,
its purpose is to present photos in a beautiful way. Wrap one or more
:html:`<figure>` elements in a :html:`<div class="m-imagegrid">` element and
delimit each row with a wrapper :html:`<div>`. Each :html:`<figure>` element
needs to contain an :html:`<img>` and a :html:`<figcaption>` with image caption
that appears on hover; these two elements can be optionally wrapped in an
:html:`<a>` to make the image clickable. If you don't want a caption, use an
empty :html:`<div>` instead of :html:`<figcaption>`. If you want the grid to
`inflate to full container width <{filename}/css/grid.rst#inflatable-nested-grid>`_,
add a :css:`.m-container-inflate` CSS class to it.

.. note-warning::

    The inner :html:`<div>` or :html:`<figcaption>` element is *important*,
    without it the grid won't look as desired.

Example usage (stupidly showing the two images all over again --- sorry):

.. code:: html

    <div class="m-imagegrid m-container-inflate">
      <div>
        <figure style="width: 69.127%">
          <a href="ship.jpg">
            <img src="ship.jpg" />
            <figcaption>F9.0, 1/250 s, ISO 100</figcaption>
          </a>
        </figure>
        <figure style="width: 30.873%">
          <a href="flowers.jpg">
            <img src="flowers.jpg" />
            <figcaption>F2.8, 1/1600 s, ISO 100</figcaption>
          </a>
        </figure>
      </div>
      <div>
        <figure style="width: 30.873%">
          <a href="flowers.jpg">
            <img src="flowers.jpg" />
            <figcaption>F2.8, 1/1600 s, ISO 100</figcaption>
          </a>
        </figure>
        <figure style="width: 69.127%">
          <a href="ship.jpg">
            <img src="ship.jpg" />
            <figcaption>F9.0, 1/250 s, ISO 100</figcaption>
          </a>
        </figure>
      </div>
    </div>

.. raw:: html

    <div class="m-imagegrid m-container-inflate">
      <div>
        <figure style="width: 69.127%">
          <a href="{static}/static/ship.jpg">
            <img src="{static}/static/ship.jpg" />
            <figcaption>F9.0, 1/250 s, ISO 100</figcaption>
          </a>
        </figure>
        <figure style="width: 30.873%">
          <a href="{static}/static/flowers.jpg">
            <img src="{static}/static/flowers.jpg" />
            <figcaption>F2.8, 1/1600 s, ISO 100</figcaption>
          </a>
        </figure>
      </div>
      <div>
        <figure style="width: 30.873%">
          <a href="{static}/static/flowers.jpg">
            <img src="{static}/static/flowers.jpg" />
            <figcaption>F2.8, 1/1600 s, ISO 100</figcaption>
          </a>
        </figure>
        <figure style="width: 69.127%">
          <a href="{static}/static/ship.jpg">
            <img src="{static}/static/ship.jpg" />
            <figcaption>F9.0, 1/250 s, ISO 100</figcaption>
          </a>
        </figure>
      </div>
    </div>

The core idea behind the image grid is scaling particular images to occupy the
same height on given row. First, a sum :math:`W` of image aspect ratios is
calculated for the whole row:

.. math::

    W = \sum_{i=0}^{n-1} \cfrac{w_i}{h_i}

Then, percentage width :math:`p_i` of each image is calculated as:

.. math::

    p_i = W \cfrac{w_i}{h_i} \cdot 100 \%

.. note-success::

    The image width calculation is quite annoying to do manually and so there's
    an :rst:`.. image-grid::` directive in the `Pelican Image plugin <{filename}/plugins/images.rst#image-grid>`_
    that does the hard work for you.

`Code`_
=======

m.css recognizes code highlighting compatible with `Pygments <http://pygments.org/>`_
and provides additional styling for it. There's a set of builtin `pygments-*.css <{filename}/css.rst>`_
styles that match the m.css themes.

For example, code highlighted using:

.. code:: sh

    echo -e "int main() {\n    return 0;\n}" | pygmentize -f html -l c++ -O nowrap

Will spit out a bunch of :html:`<span>` elements like below. To create a code
block, wrap the output in :html:`<pre class="m-code">` (note that whitespace
matters inside this tag). The block doesn't wrap lines on narrow screens to not
hurt readability, a horizontal scrollbar is shown instead if the content is
too wide.

.. code-figure::

    .. code:: html

        <pre class="m-code"><span class="kt">int</span> <span class="nf">main</span><span class="p">()</span> <span class="p">{</span>
            <span class="k">return</span> <span class="mi">0</span><span class="p">;</span>
        <span class="p">}</span></pre>

    .. raw:: html

        <pre class="m-code"><span class="kt">int</span> <span class="nf">main</span><span class="p">()</span> <span class="p">{</span>
            <span class="k">return</span> <span class="mi">0</span><span class="p">;</span>
        <span class="p">}</span></pre>

Pygments allow to highlight arbitrary lines, which affect the rendering in this
way:

.. code:: c++
    :hl_lines: 2 3

    int main() {
        std::cout << "Hello world!" << std::endl;
        return 0;
    }

Sometimes you want to focus on code that has been changed / added and mute the
rest. Add an additional :css:`.m-inverted` CSS class to the
:html:`<pre class="m-code">` to achieve this effect:

.. code:: c++
    :hl_lines: 4 5
    :class: m-inverted

    #include <iostream>

    int main() {
        std::cout << "Hello world!" << std::endl;
        return 0;
    }

To have code highlighting inline, wrap the output in :html:`<code class="m-code">`
instead of :html:`<pre>`:

.. code-figure::

    .. code:: html

        <p>The <code class="m-code"><span class="n">main</span><span class="p">()</span></code>
        function doesn't need to have a <code class="m-code"><span class="k">return</span></code>
        statement.</p>

    .. raw:: html

        <p>The <code class="m-code"><span class="n">main</span><span class="p">()</span></code>
        function doesn't need to have a <code class="m-code"><span class="k">return</span></code>
        statement.</p>

.. note-success::

    To make your life easier, the `Pelican Code plugin <{filename}/plugins/math-and-code.rst#code>`__
    integrates Pygments code highlighting as a :rst:`.. code::`
    :abbr:`reST <reStructuredText>` directive and a :rst:`:code:` inline text
    role.

`Color swatches in code snippets`_
----------------------------------

For code dealing with colors it might be useful to show the actual color that's
being represented by a hexadecimal literal, for example. In the below snippet,
:html:`<span class="m-code-color" style="background-color: #3bd267;"></span>`
is added next to the literal, showing a colored square:

.. code-figure::

    .. code:: html

        <pre class="m-code"><span class="p">.</span><span class="nc">success</span> <span class="p">{</span>
          <span class="k">color</span><span class="p">:</span> <span class="mh">#3bd267<span class="m-code-color" style="background-color: #3bd267;"></span></span><span class="p">;</span>
        <span class="p">}</span></pre>

    .. raw:: html

        <pre class="m-code"><span class="p">.</span><span class="nc">success</span> <span class="p">{</span>
          <span class="k">color</span><span class="p">:</span> <span class="mh">#3bd267<span class="m-code-color" style="background-color: #3bd267;"></span></span><span class="p">;</span>
        <span class="p">}</span></pre>

`Colored terminal output`_
--------------------------

Besides code, it's also possible to "highlight" ANSI-colored terminal output.
For that, m.css provides a custom Pygments lexer that's together with
`pygments-console.css <{filename}/css.rst>`_ able to detect and highlight the
basic 4-bit color codes (8 foreground colors in either normal or bright
version) and a tiny subset of the 24-bit color scheme as well. Download the
:gh:`ansilexer.py <mosra/m.css$master/plugins/m/ansilexer.py>` file or use it
directly from your Git clone of m.css. Example usage:

.. code:: sh

    ls -C --color=always | pygmentize -l plugins/ansilexer.py:AnsiLexer -x -f html -O nowrap

Wrap the HTML output in either :html:`<pre class="m-console">` for a block
listing or :html:`<code class="m-console">` for inline listing. The output
might then look similarly to this:

.. code-figure::

    .. code:: html

        <pre class="m-console">CONTRIBUTING.rst  CREDITS.rst  <span class="g g-AnsiBrightBlue">doc</span>            <span class="g g-AnsiBrightBlue">plugins</span>        README.rst
        COPYING           <span class="g g-AnsiBrightBlue">css</span>          <span class="g g-AnsiBrightBlue">documentation</span>  <span class="g g-AnsiBrightBlue">pelican-theme</span>  <span class="g g-AnsiBrightBlue">site</span></pre>

    .. raw:: html

        <pre class="m-console">CONTRIBUTING.rst  CREDITS.rst  <span class="g g-AnsiBrightBlue">doc</span>            <span class="g g-AnsiBrightBlue">plugins</span>        README.rst
        COPYING           <span class="g g-AnsiBrightBlue">css</span>          <span class="g g-AnsiBrightBlue">documentation</span>  <span class="g g-AnsiBrightBlue">pelican-theme</span>  <span class="g g-AnsiBrightBlue">site</span></pre>

It's sometimes desirable to have console output wrapped to the available
container width (like terminals do). Add :css:`.m-console-wrap` to the
:html:`<pre>` to achieve that effect.

.. note-success::

    The Pelican Code plugin mentioned above is able to do
    `colored console highlighting as well <{filename}/plugins/math-and-code.rst#colored-terminal-output>`_.

`Code figure`_
--------------

It often happens that you want to present code with corresponding result
together. The code figure looks similar to `image figures <#figures>`_ and
consists of a :html:`<figure>` (or :html:`<div>`) element with the
:css:`.m-code-figure` class containing a :html:`<pre>` block and whatever else
you want to put in as the result. The :html:`<pre>` element has to be the very
first child of the :html:`<figure>` for the markup to work correctly. Similarly
to image figure, you can apply the :css:`.m-flat` CSS class to remove the
border, the :html:`<figcaption>` element is styled as well.

Example (note that this page uses code figure for all code examples, so it's a
bit of a figure inception shown here):

.. code-figure::

    .. code:: html

        <figure class="m-code-figure">
          <pre>Some
            code
        snippet</pre>
          And a resulting output.
        </figure>

    .. raw:: html

        <figure class="m-code-figure">
          <pre>Some
            code
        snippet</pre>
          And a resulting output.
        </figure>

It's also possible to have matching border for a console output. Just use
:css:`.m-console-figure` instead of :css:`.m-code-figure` on the outer element.
For reduced clutter when combining a code figure with console output (and vice
versa), mark the second :html:`<pre>` with :css:`.m-nopad`:

.. code-figure::

    .. code:: html

        <figure class="m-code-figure">
            <pre class="m-code">Some
            code
        snippet</pre>
            <pre class="m-console m-nopad">And a resulting output.</pre>
        </figure>

    .. raw:: html

        <figure class="m-code-figure">
            <pre class="m-code">Some
            code
        snippet</pre>
            <pre class="m-console m-nopad">And a resulting output.</pre>
        </figure>

.. note-info::

    Code figures can be conveniently created with a :rst:`.. code-figure::`
    directive in your :abbr:`reST <reStructuredText>` markup using the
    `Pelican Components plugin <{filename}/plugins/components.rst#code-math-and-graph-figure>`_.

`Math`_
=======

The ``latex2svg.py`` utility from :gh:`tuxu/latex2svg` can be used to generate
SVG representation of your LaTeX math formulas. Assuming you have some LaTeX
distribution and `dvisvgm <https://dvisvgm.de/>`_ installed, the usage is:

.. code:: sh

    echo "\$\$ a^2 = b^2 + c^2 \$\$" | python plugins/latex2svg.py > formula.svg

The ``formula.svg`` file will then contain the rendered formula, which with
some minor patching (removing the XML preamble etc.) can be pasted directly
into your HTML code. The :html:`<svg>` element containing math can be wrapped
in :html:`<div class="m-math">` to make it centered. Similarly to code
blocks, the math block shows a horizontal scrollbar if the content is too wide
on narrow screens. `CSS color classes <#colors>`_ can be applied to the
:html:`<div>`. It's a good practice to include the :html:`<title>` element for accessibility reasons.

.. code-figure::

    .. code:: html

        <div class="m-math m-success">
          <svg>
            <title>a^2 = b^2 + c^2</title>
            <g>...</g>
          </svg>
        </div>

    .. raw:: html

        <div class="m-math m-success">
          <svg height='13.7321pt' version='1.1' viewBox='164.011 -10.9857 60.0231 10.9857' width='75.0289pt'>
            <title>a^2 = b^2 + c^2</title>
            <defs>
              <path d='M3.59851 -1.42267C3.53873 -1.21943 3.53873 -1.19552 3.37136 -0.968369C3.10834 -0.633624 2.58232 -0.119552 2.02042 -0.119552C1.53026 -0.119552 1.25529 -0.561893 1.25529 -1.26725C1.25529 -1.92478 1.6259 -3.26376 1.85305 -3.76588C2.25953 -4.60274 2.82142 -5.03313 3.28767 -5.03313C4.07671 -5.03313 4.23213 -4.0528 4.23213 -3.95716C4.23213 -3.94521 4.19626 -3.78979 4.18431 -3.76588L3.59851 -1.42267ZM4.36364 -4.48319C4.23213 -4.79402 3.90934 -5.27223 3.28767 -5.27223C1.93674 -5.27223 0.478207 -3.52677 0.478207 -1.75741C0.478207 -0.573848 1.17161 0.119552 1.98456 0.119552C2.64209 0.119552 3.20399 -0.394521 3.53873 -0.789041C3.65828 -0.0836862 4.22017 0.119552 4.57883 0.119552S5.22441 -0.0956413 5.4396 -0.526027C5.63088 -0.932503 5.79826 -1.66177 5.79826 -1.70959C5.79826 -1.76936 5.75044 -1.81719 5.6787 -1.81719C5.57111 -1.81719 5.55915 -1.75741 5.51133 -1.57808C5.332 -0.872727 5.10486 -0.119552 4.61469 -0.119552C4.268 -0.119552 4.24408 -0.430386 4.24408 -0.669489C4.24408 -0.944458 4.27995 -1.07597 4.38755 -1.54222C4.47123 -1.8411 4.53101 -2.10411 4.62665 -2.45081C5.06899 -4.24408 5.17659 -4.67447 5.17659 -4.7462C5.17659 -4.91357 5.04508 -5.04508 4.86575 -5.04508C4.48319 -5.04508 4.38755 -4.62665 4.36364 -4.48319Z' id='math1-g0-97'/>
              <path d='M2.76164 -7.99801C2.7736 -8.04583 2.79751 -8.11756 2.79751 -8.17733C2.79751 -8.29689 2.67796 -8.29689 2.65405 -8.29689C2.64209 -8.29689 2.21171 -8.26102 1.99651 -8.23711C1.79328 -8.22516 1.61395 -8.20125 1.39875 -8.18929C1.11183 -8.16538 1.02814 -8.15342 1.02814 -7.93823C1.02814 -7.81868 1.1477 -7.81868 1.26725 -7.81868C1.87696 -7.81868 1.87696 -7.71108 1.87696 -7.59153C1.87696 -7.50785 1.78132 -7.16115 1.7335 -6.94595L1.44658 -5.79826C1.32702 -5.32005 0.645579 -2.60623 0.597758 -2.39103C0.537983 -2.09215 0.537983 -1.88892 0.537983 -1.7335C0.537983 -0.514072 1.21943 0.119552 1.99651 0.119552C3.38331 0.119552 4.81793 -1.66177 4.81793 -3.39527C4.81793 -4.49514 4.19626 -5.27223 3.29963 -5.27223C2.67796 -5.27223 2.11606 -4.75816 1.88892 -4.51905L2.76164 -7.99801ZM2.00847 -0.119552C1.6259 -0.119552 1.20747 -0.406476 1.20747 -1.33898C1.20747 -1.7335 1.24334 -1.96065 1.45853 -2.79751C1.4944 -2.95293 1.68568 -3.71806 1.7335 -3.87347C1.75741 -3.96912 2.46276 -5.03313 3.27572 -5.03313C3.80174 -5.03313 4.04085 -4.5071 4.04085 -3.88543C4.04085 -3.31158 3.7061 -1.96065 3.40722 -1.33898C3.10834 -0.6934 2.55841 -0.119552 2.00847 -0.119552Z' id='math1-g0-98'/>
              <path d='M4.67447 -4.49514C4.44732 -4.49514 4.33973 -4.49514 4.17235 -4.35168C4.10062 -4.29191 3.96912 -4.11258 3.96912 -3.9213C3.96912 -3.68219 4.14844 -3.53873 4.37559 -3.53873C4.66252 -3.53873 4.98531 -3.77783 4.98531 -4.25604C4.98531 -4.82989 4.43537 -5.27223 3.61046 -5.27223C2.04433 -5.27223 0.478207 -3.56264 0.478207 -1.86501C0.478207 -0.824907 1.12379 0.119552 2.34321 0.119552C3.96912 0.119552 4.99726 -1.1477 4.99726 -1.30311C4.99726 -1.37484 4.92553 -1.43462 4.87771 -1.43462C4.84184 -1.43462 4.82989 -1.42267 4.72229 -1.31507C3.95716 -0.298879 2.82142 -0.119552 2.36712 -0.119552C1.54222 -0.119552 1.2792 -0.836862 1.2792 -1.43462C1.2792 -1.85305 1.48244 -3.0127 1.91283 -3.82565C2.22366 -4.38755 2.86924 -5.03313 3.62242 -5.03313C3.77783 -5.03313 4.43537 -5.00922 4.67447 -4.49514Z' id='math1-g0-99'/>
              <path d='M4.77011 -2.76164H8.06974C8.23711 -2.76164 8.4523 -2.76164 8.4523 -2.97684C8.4523 -3.20399 8.24907 -3.20399 8.06974 -3.20399H4.77011V-6.50361C4.77011 -6.67098 4.77011 -6.88618 4.55492 -6.88618C4.32777 -6.88618 4.32777 -6.68294 4.32777 -6.50361V-3.20399H1.02814C0.860772 -3.20399 0.645579 -3.20399 0.645579 -2.98879C0.645579 -2.76164 0.848817 -2.76164 1.02814 -2.76164H4.32777V0.537983C4.32777 0.705355 4.32777 0.920548 4.54296 0.920548C4.77011 0.920548 4.77011 0.71731 4.77011 0.537983V-2.76164Z' id='math1-g2-43'/>
              <path d='M8.06974 -3.87347C8.23711 -3.87347 8.4523 -3.87347 8.4523 -4.08867C8.4523 -4.31582 8.24907 -4.31582 8.06974 -4.31582H1.02814C0.860772 -4.31582 0.645579 -4.31582 0.645579 -4.10062C0.645579 -3.87347 0.848817 -3.87347 1.02814 -3.87347H8.06974ZM8.06974 -1.64981C8.23711 -1.64981 8.4523 -1.64981 8.4523 -1.86501C8.4523 -2.09215 8.24907 -2.09215 8.06974 -2.09215H1.02814C0.860772 -2.09215 0.645579 -2.09215 0.645579 -1.87696C0.645579 -1.64981 0.848817 -1.64981 1.02814 -1.64981H8.06974Z' id='math1-g2-61'/>
              <path d='M2.24757 -1.6259C2.37509 -1.74545 2.70984 -2.00847 2.83736 -2.12005C3.33151 -2.57435 3.80174 -3.0127 3.80174 -3.73798C3.80174 -4.68643 3.00473 -5.30012 2.00847 -5.30012C1.05205 -5.30012 0.422416 -4.57484 0.422416 -3.8655C0.422416 -3.47497 0.73325 -3.41918 0.844832 -3.41918C1.0122 -3.41918 1.25928 -3.53873 1.25928 -3.84159C1.25928 -4.25604 0.860772 -4.25604 0.765131 -4.25604C0.996264 -4.83786 1.53026 -5.03711 1.9208 -5.03711C2.66202 -5.03711 3.04458 -4.40747 3.04458 -3.73798C3.04458 -2.90909 2.46276 -2.30336 1.52229 -1.33898L0.518057 -0.302864C0.422416 -0.215193 0.422416 -0.199253 0.422416 0H3.57061L3.80174 -1.42665H3.55467C3.53076 -1.26725 3.467 -0.868742 3.37136 -0.71731C3.32354 -0.653549 2.71781 -0.653549 2.59029 -0.653549H1.17161L2.24757 -1.6259Z' id='math1-g1-50'/>
            </defs>
            <g id='math1-page1'>
              <use x='164.011' xlink:href='#math1-g0-97' y='-0.913201'/>
              <use x='170.156' xlink:href='#math1-g1-50' y='-5.84939'/>
              <use x='178.209' xlink:href='#math1-g2-61' y='-0.913201'/>
              <use x='190.634' xlink:href='#math1-g0-98' y='-0.913201'/>
              <use x='195.612' xlink:href='#math1-g1-50' y='-5.84939'/>
              <use x='203.001' xlink:href='#math1-g2-43' y='-0.913201'/>
              <use x='214.762' xlink:href='#math1-g0-99' y='-0.913201'/>
              <use x='219.8' xlink:href='#math1-g1-50' y='-5.84939'/>
            </g>
          </svg>
        </div>

For inline math, add the :css:`.m-math` class to the :html:`<svg>` tag
directly. Note that you'll probably need to manually specify
:css:`vertical-align` style to make the formula align well with surrounding
text. You can use CSS color classes here as well. When using the ``latex2svg.py``
utility, wrap the formula in ``$`` instead of ``$$`` to produce inline math;
the ``depth`` value returned on stderr can be taken as a base for the
:css:`vertical-align` property.

.. code-figure::

    .. code:: html

        <p>There is <a href="https://tauday.com/">a movement</a> for replacing circle
        constant <svg class="m-math" style="vertical-align: 0.0pt">...</svg> with the
        <svg class="m-math m-warning" style="vertical-align: 0.0pt">...</svg> character.

    .. raw:: html

        <p>There is <a href="https://tauday.com/">a movement</a> for replacing
        circle constant <svg class="m-math" style="vertical-align: -0.0pt;" height='9.63055pt' version='1.1' viewBox='0 -7.70444 12.9223 7.70444' width='16.1528pt'>
          <title>2 \pi</title>
          <defs>
            <path d='M3.09639 -4.5071H4.44732C4.12453 -3.16812 3.9213 -2.29539 3.9213 -1.33898C3.9213 -1.17161 3.9213 0.119552 4.41146 0.119552C4.66252 0.119552 4.87771 -0.107597 4.87771 -0.310834C4.87771 -0.37061 4.87771 -0.394521 4.79402 -0.573848C4.47123 -1.39875 4.47123 -2.4269 4.47123 -2.51059C4.47123 -2.58232 4.47123 -3.43113 4.72229 -4.5071H6.06127C6.21669 -4.5071 6.61121 -4.5071 6.61121 -4.88966C6.61121 -5.15268 6.38406 -5.15268 6.16887 -5.15268H2.23562C1.96065 -5.15268 1.55417 -5.15268 1.00423 -4.56687C0.6934 -4.22017 0.310834 -3.58655 0.310834 -3.51482S0.37061 -3.41918 0.442341 -3.41918C0.526027 -3.41918 0.537983 -3.45504 0.597758 -3.52677C1.21943 -4.5071 1.8411 -4.5071 2.13998 -4.5071H2.82142C2.55841 -3.61046 2.25953 -2.57036 1.2792 -0.478207C1.18356 -0.286924 1.18356 -0.263014 1.18356 -0.191283C1.18356 0.0597758 1.39875 0.119552 1.50635 0.119552C1.85305 0.119552 1.94869 -0.191283 2.09215 -0.6934C2.28344 -1.30311 2.28344 -1.32702 2.40299 -1.80523L3.09639 -4.5071Z' id='math2-g0-25'/>
            <path d='M5.26027 -2.00847H4.99726C4.96139 -1.80523 4.86575 -1.1477 4.7462 -0.956413C4.66252 -0.848817 3.98107 -0.848817 3.62242 -0.848817H1.41071C1.7335 -1.12379 2.46276 -1.88892 2.7736 -2.17584C4.59078 -3.84956 5.26027 -4.47123 5.26027 -5.65479C5.26027 -7.02964 4.17235 -7.95019 2.78555 -7.95019S0.585803 -6.76663 0.585803 -5.73848C0.585803 -5.12877 1.11183 -5.12877 1.1477 -5.12877C1.39875 -5.12877 1.70959 -5.30809 1.70959 -5.69066C1.70959 -6.0254 1.48244 -6.25255 1.1477 -6.25255C1.0401 -6.25255 1.01619 -6.25255 0.980324 -6.2406C1.20747 -7.05355 1.85305 -7.60349 2.63014 -7.60349C3.64633 -7.60349 4.268 -6.75467 4.268 -5.65479C4.268 -4.63861 3.68219 -3.75392 3.00075 -2.98879L0.585803 -0.286924V0H4.94944L5.26027 -2.00847Z' id='math2-g1-50'/>
          </defs>
          <g id='math2-page1'>
            <use x='0' xlink:href='#math2-g1-50' y='0'/>
            <use x='5.85299' xlink:href='#math2-g0-25' y='0'/>
          </g>
        </svg> with the <svg class="m-math m-warning" style="vertical-align: -0.0pt;" height='6.43422pt' version='1.1' viewBox='0 -5.14737 6.41894 5.14737' width='8.02368pt'>
          <title>\tau</title>
          <defs>
            <path d='M3.43113 -4.5071H5.41569C5.57111 -4.5071 5.96563 -4.5071 5.96563 -4.88966C5.96563 -5.15268 5.73848 -5.15268 5.52329 -5.15268H2.23562C1.96065 -5.15268 1.55417 -5.15268 1.00423 -4.56687C0.6934 -4.22017 0.310834 -3.58655 0.310834 -3.51482S0.37061 -3.41918 0.442341 -3.41918C0.526027 -3.41918 0.537983 -3.45504 0.597758 -3.52677C1.21943 -4.5071 1.8411 -4.5071 2.13998 -4.5071H3.13225L1.88892 -0.406476C1.82914 -0.227148 1.82914 -0.203238 1.82914 -0.167372C1.82914 -0.0358655 1.91283 0.131507 2.15193 0.131507C2.52254 0.131507 2.58232 -0.191283 2.61818 -0.37061L3.43113 -4.5071Z' id='math3-g0-28'/>
          </defs>
          <g id='math3-page1'>
            <use x='0' xlink:href='#math3-g0-28' y='0'/>
          </g>
        </svg> character.</p>

The CSS color classes work also on :html:`<g>` and :html:`<rect>` elements
inside :html:`<svg>` for highlighting parts of formulas.

.. note-success::

    Producing SVG manually using command-line tools is no fun. That's why the
    :rst:`.. math::` directive and :rst:`:math:` inline text role in the
    `Pelican Math plugin <{filename}/plugins/math-and-code.rst#math>`__
    integrates LaTeX math directly into your :abbr:`reST <reStructuredText>`
    markup for convenient content authoring.

`Math figure`_
--------------

Similarly to `code figure`_, math can be also put in a :html:`<figure>` with
assigned caption and description. It behaves the same as `image figures`_, the
figure width being defined by the math equation size. Create a
:html:`<figure class="m-figure">` element and put :html:`<svg class="m-math">`
as a first child. The remaining content of the figure can be :html:`<figcaption>`
and/or arbitrary other markup. Add the :css:`.m-flat` class to the
:html:`<figure>` to remove the outer border and equation background,
`CSS color classes`_ on the :html:`<figure>` affect the figure, on the
:html:`<svg>` affect the equation.

.. code-figure::

    .. code:: html

        <figure class="m-figure">
          <svg class="m-math">
            <title>a^2 = b^2 + c^2</title>
            <g>...</g>
          </svg>
          <figcaption>Pythagorean theorem</figcaption>
        </figure>

    .. raw:: html

        <figure class="m-figure">
          <svg class="m-math" height='13.7321pt' version='1.1' viewBox='164.011 -10.9857 60.0231 10.9857' width='75.0289pt'>
            <title>a^2 = b^2 + c^2</title>
            <defs>
              <path d='M3.59851 -1.42267C3.53873 -1.21943 3.53873 -1.19552 3.37136 -0.968369C3.10834 -0.633624 2.58232 -0.119552 2.02042 -0.119552C1.53026 -0.119552 1.25529 -0.561893 1.25529 -1.26725C1.25529 -1.92478 1.6259 -3.26376 1.85305 -3.76588C2.25953 -4.60274 2.82142 -5.03313 3.28767 -5.03313C4.07671 -5.03313 4.23213 -4.0528 4.23213 -3.95716C4.23213 -3.94521 4.19626 -3.78979 4.18431 -3.76588L3.59851 -1.42267ZM4.36364 -4.48319C4.23213 -4.79402 3.90934 -5.27223 3.28767 -5.27223C1.93674 -5.27223 0.478207 -3.52677 0.478207 -1.75741C0.478207 -0.573848 1.17161 0.119552 1.98456 0.119552C2.64209 0.119552 3.20399 -0.394521 3.53873 -0.789041C3.65828 -0.0836862 4.22017 0.119552 4.57883 0.119552S5.22441 -0.0956413 5.4396 -0.526027C5.63088 -0.932503 5.79826 -1.66177 5.79826 -1.70959C5.79826 -1.76936 5.75044 -1.81719 5.6787 -1.81719C5.57111 -1.81719 5.55915 -1.75741 5.51133 -1.57808C5.332 -0.872727 5.10486 -0.119552 4.61469 -0.119552C4.268 -0.119552 4.24408 -0.430386 4.24408 -0.669489C4.24408 -0.944458 4.27995 -1.07597 4.38755 -1.54222C4.47123 -1.8411 4.53101 -2.10411 4.62665 -2.45081C5.06899 -4.24408 5.17659 -4.67447 5.17659 -4.7462C5.17659 -4.91357 5.04508 -5.04508 4.86575 -5.04508C4.48319 -5.04508 4.38755 -4.62665 4.36364 -4.48319Z' id='math1-g0-97'/>
              <path d='M2.76164 -7.99801C2.7736 -8.04583 2.79751 -8.11756 2.79751 -8.17733C2.79751 -8.29689 2.67796 -8.29689 2.65405 -8.29689C2.64209 -8.29689 2.21171 -8.26102 1.99651 -8.23711C1.79328 -8.22516 1.61395 -8.20125 1.39875 -8.18929C1.11183 -8.16538 1.02814 -8.15342 1.02814 -7.93823C1.02814 -7.81868 1.1477 -7.81868 1.26725 -7.81868C1.87696 -7.81868 1.87696 -7.71108 1.87696 -7.59153C1.87696 -7.50785 1.78132 -7.16115 1.7335 -6.94595L1.44658 -5.79826C1.32702 -5.32005 0.645579 -2.60623 0.597758 -2.39103C0.537983 -2.09215 0.537983 -1.88892 0.537983 -1.7335C0.537983 -0.514072 1.21943 0.119552 1.99651 0.119552C3.38331 0.119552 4.81793 -1.66177 4.81793 -3.39527C4.81793 -4.49514 4.19626 -5.27223 3.29963 -5.27223C2.67796 -5.27223 2.11606 -4.75816 1.88892 -4.51905L2.76164 -7.99801ZM2.00847 -0.119552C1.6259 -0.119552 1.20747 -0.406476 1.20747 -1.33898C1.20747 -1.7335 1.24334 -1.96065 1.45853 -2.79751C1.4944 -2.95293 1.68568 -3.71806 1.7335 -3.87347C1.75741 -3.96912 2.46276 -5.03313 3.27572 -5.03313C3.80174 -5.03313 4.04085 -4.5071 4.04085 -3.88543C4.04085 -3.31158 3.7061 -1.96065 3.40722 -1.33898C3.10834 -0.6934 2.55841 -0.119552 2.00847 -0.119552Z' id='math1-g0-98'/>
              <path d='M4.67447 -4.49514C4.44732 -4.49514 4.33973 -4.49514 4.17235 -4.35168C4.10062 -4.29191 3.96912 -4.11258 3.96912 -3.9213C3.96912 -3.68219 4.14844 -3.53873 4.37559 -3.53873C4.66252 -3.53873 4.98531 -3.77783 4.98531 -4.25604C4.98531 -4.82989 4.43537 -5.27223 3.61046 -5.27223C2.04433 -5.27223 0.478207 -3.56264 0.478207 -1.86501C0.478207 -0.824907 1.12379 0.119552 2.34321 0.119552C3.96912 0.119552 4.99726 -1.1477 4.99726 -1.30311C4.99726 -1.37484 4.92553 -1.43462 4.87771 -1.43462C4.84184 -1.43462 4.82989 -1.42267 4.72229 -1.31507C3.95716 -0.298879 2.82142 -0.119552 2.36712 -0.119552C1.54222 -0.119552 1.2792 -0.836862 1.2792 -1.43462C1.2792 -1.85305 1.48244 -3.0127 1.91283 -3.82565C2.22366 -4.38755 2.86924 -5.03313 3.62242 -5.03313C3.77783 -5.03313 4.43537 -5.00922 4.67447 -4.49514Z' id='math1-g0-99'/>
              <path d='M4.77011 -2.76164H8.06974C8.23711 -2.76164 8.4523 -2.76164 8.4523 -2.97684C8.4523 -3.20399 8.24907 -3.20399 8.06974 -3.20399H4.77011V-6.50361C4.77011 -6.67098 4.77011 -6.88618 4.55492 -6.88618C4.32777 -6.88618 4.32777 -6.68294 4.32777 -6.50361V-3.20399H1.02814C0.860772 -3.20399 0.645579 -3.20399 0.645579 -2.98879C0.645579 -2.76164 0.848817 -2.76164 1.02814 -2.76164H4.32777V0.537983C4.32777 0.705355 4.32777 0.920548 4.54296 0.920548C4.77011 0.920548 4.77011 0.71731 4.77011 0.537983V-2.76164Z' id='math1-g2-43'/>
              <path d='M8.06974 -3.87347C8.23711 -3.87347 8.4523 -3.87347 8.4523 -4.08867C8.4523 -4.31582 8.24907 -4.31582 8.06974 -4.31582H1.02814C0.860772 -4.31582 0.645579 -4.31582 0.645579 -4.10062C0.645579 -3.87347 0.848817 -3.87347 1.02814 -3.87347H8.06974ZM8.06974 -1.64981C8.23711 -1.64981 8.4523 -1.64981 8.4523 -1.86501C8.4523 -2.09215 8.24907 -2.09215 8.06974 -2.09215H1.02814C0.860772 -2.09215 0.645579 -2.09215 0.645579 -1.87696C0.645579 -1.64981 0.848817 -1.64981 1.02814 -1.64981H8.06974Z' id='math1-g2-61'/>
              <path d='M2.24757 -1.6259C2.37509 -1.74545 2.70984 -2.00847 2.83736 -2.12005C3.33151 -2.57435 3.80174 -3.0127 3.80174 -3.73798C3.80174 -4.68643 3.00473 -5.30012 2.00847 -5.30012C1.05205 -5.30012 0.422416 -4.57484 0.422416 -3.8655C0.422416 -3.47497 0.73325 -3.41918 0.844832 -3.41918C1.0122 -3.41918 1.25928 -3.53873 1.25928 -3.84159C1.25928 -4.25604 0.860772 -4.25604 0.765131 -4.25604C0.996264 -4.83786 1.53026 -5.03711 1.9208 -5.03711C2.66202 -5.03711 3.04458 -4.40747 3.04458 -3.73798C3.04458 -2.90909 2.46276 -2.30336 1.52229 -1.33898L0.518057 -0.302864C0.422416 -0.215193 0.422416 -0.199253 0.422416 0H3.57061L3.80174 -1.42665H3.55467C3.53076 -1.26725 3.467 -0.868742 3.37136 -0.71731C3.32354 -0.653549 2.71781 -0.653549 2.59029 -0.653549H1.17161L2.24757 -1.6259Z' id='math1-g1-50'/>
            </defs>
            <g id='math1-page1'>
              <use x='164.011' xlink:href='#math1-g0-97' y='-0.913201'/>
              <use x='170.156' xlink:href='#math1-g1-50' y='-5.84939'/>
              <use x='178.209' xlink:href='#math1-g2-61' y='-0.913201'/>
              <use x='190.634' xlink:href='#math1-g0-98' y='-0.913201'/>
              <use x='195.612' xlink:href='#math1-g1-50' y='-5.84939'/>
              <use x='203.001' xlink:href='#math1-g2-43' y='-0.913201'/>
              <use x='214.762' xlink:href='#math1-g0-99' y='-0.913201'/>
              <use x='219.8' xlink:href='#math1-g1-50' y='-5.84939'/>
            </g>
          </svg>
          <figcaption>Pythagorean theorem</figcaption>
        </figure>

`Plots`_
========

Wrap a :html:`<svg>` element in a :html:`<div class="m-plot">` to make it
centered and occupying full width. Mark plot axes background with
:css:`.m-background`, bars can be styled using :css:`.m-bar` and a
corresponding `CSS color class <#colors>`_. Mark ticks and various other lines
with :css:`.m-line`, error bars with :css:`.m-error`. Use
:html:`<text class="m-label">` for tick and axes labels and
:html:`<text class="m-title">` for graph title.

.. code-figure::

    .. code:: html

        <div class="m-plot"><svg>
          <path d="M 68.22875 70.705312 ..." class="m-background"/>
          <path d="M 68.22875 29.116811 ..." class="m-bar m-warning"/>
          <path d="M 68.22875 51.121309 ..." class="m-bar m-primary"/>
          ...
          <defs><path d="..." id="mba4ce04b6c" class="m-line"/></defs>
          <use x="68.22875" xlink:href="#mba4ce04b6c" y="37.91861"/>
          <text class="m-label" style="text-anchor:end;" ...>Cheetah</text>
          ...
          <path d="M 428.616723 37.91861 ..." class="m-error"/>
          ...
          <text class="m-title" style="text-anchor:middle;" ...>Fastest animals</text>
        </svg></div>

    .. container:: m-plot

        .. raw:: html
            :file: components-plot.svg

.. note-info::

    Plot styling is designed to be used with extenal tools, for example Python
    Matplotlib. If you use Pelican, m.css has a
    `m.plots <{filename}/plugins/plots-and-graphs.rst#plots>`__ plugin that
    allows you to produce plots using :rst:`.. plot::` directives directly in
    your :abbr:`reST <reStructuredText>` markup.

`Graphs`_
=========

Wrap a :html:`<svg>` element in a :html:`<div class="m-graph">` to make it
centered and occupying full width at most. Wrap edge :html:`<path>`,
:html:`<polygon>` and :html:`<text>` elements in :html:`<g class="m-edge">` to
style them as edges, wrap node :html:`<polygon>`, :html:`<ellipse>` and
:html:`<text>` elements in :html:`<g class="m-node">` to style them as nodes.
You can use `CSS color classes <#colors>`_ on either the wrapper :html:`<div>`
or on the :html:`<g>` to color the whole graph or its parts. Use :css:`.m-flat`
on a :css:`.m-node` to make it just an outline instead of filled.

.. code-figure::

    .. code:: html

        <div class="m-graph m-info"><svg>
          <g class="m-node">
            <ellipse cx="27.5772" cy="-27.5772" rx="27.6545" ry="27.6545"/>
            <text text-anchor="middle" x="27.5772" y="-23.7772">yes</text>
          </g>
          <g class="m-node m-flat">
            <ellipse cx="134.9031" cy="-27.5772" rx="25" ry="25"/>
            <text text-anchor="middle" x="134.9031" y="-23.7772">no</text>
          </g>
          <g class="m-edge">
            <path d="M55.2163,-27.5772C68.8104,-27.5772 85.3444,-27.5772 99.8205,-27.5772"/>
            <polygon points="99.9261,-31.0773 109.9261,-27.5772 99.9261,-24.0773 99.9261,-31.0773"/>
            <text text-anchor="middle" x="82.6543" y="-32.7772">no</text>
          </g>
          <g class="m-edge m-dim">
            <path d="M125.3459,-50.4471C124.3033,-61.0564 127.489,-70.3259 134.9031,-70.3259 139.7685,-70.3259 142.813,-66.3338 144.0365,-60.5909"/>
            <polygon points="147.5398,-60.5845 144.4603,-50.4471 140.5459,-60.2923 147.5398,-60.5845"/>
            <text text-anchor="middle" x="134.9031" y="-75.5259">no</text>
          </g>
        </svg></div>

    .. raw:: html

        <div class="m-graph m-info">
        <svg style="width: 10.500rem; height: 6.000rem;" viewBox="0.00 0.00 167.65 96.33">
        <g transform="scale(1 1) rotate(0) translate(4 92.3259)">
        <title>FSM</title>
        <g class="m-node">
        <title>yes</title>
        <ellipse cx="27.5772" cy="-27.5772" rx="27.6545" ry="27.6545"/>
        <text text-anchor="middle" x="27.5772" y="-23.7772">yes</text>
        </g>
        <g class="m-node m-flat">
        <title>no</title>
        <ellipse cx="134.9031" cy="-27.5772" rx="25" ry="25"/>
        <text text-anchor="middle" x="134.9031" y="-23.7772">no</text>
        </g>
        <g class="m-edge">
        <title>yes&#45;&gt;no</title>
        <path d="M55.2163,-27.5772C68.8104,-27.5772 85.3444,-27.5772 99.8205,-27.5772"/>
        <polygon points="99.9261,-31.0773 109.9261,-27.5772 99.9261,-24.0773 99.9261,-31.0773"/>
        <text text-anchor="middle" x="82.6543" y="-32.7772">no</text>
        </g>
        <g class="m-edge m-dim">
        <title>no&#45;&gt;no</title>
        <path d="M125.3459,-50.4471C124.3033,-61.0564 127.489,-70.3259 134.9031,-70.3259 139.7685,-70.3259 142.813,-66.3338 144.0365,-60.5909"/>
        <polygon points="147.5398,-60.5845 144.4603,-50.4471 140.5459,-60.2923 147.5398,-60.5845"/>
        <text text-anchor="middle" x="134.9031" y="-75.5259">no</text>
        </g>
        </g>
        </svg>
        </div>

.. note-primary::

    Similarly to plot styling, graph styling is designed to be used with
    external tools, for example Graphviz. If you use Pelican, m.css has a
    `m.dot <{filename}/plugins/plots-and-graphs.rst#graphs>`__ plugin that
    allows you to produce plots using :rst:`.. graph::` directives directly in
    your :abbr:`reST <reStructuredText>` markup.

`Graph figure`_
---------------

Similarly to `math figure`_, graphs also can be :html:`<figure>`\ s. The
behavior is almost identical, create a :html:`<figure class="m-figure m-graph">`
element and put the :html:`<svg>` as a first child, all other content after.

.. code-figure::

    .. code:: html

        <figure class="m-figure">
          <svg class="m-graph m-warning">
            ...
          </svg>
          <figcaption>Impenetrable logic</figcaption>
        </figure>

    .. raw:: html

        <figure class="m-figure">
        <svg class="m-graph m-warning" style="width: 10.500rem; height: 6.000rem;" viewBox="0.00 0.00 167.65 96.33">
        <g transform="scale(1 1) rotate(0) translate(4 92.3259)">
        <title>FSM</title>
        <g class="m-node">
        <title>yes</title>
        <ellipse cx="27.5772" cy="-27.5772" rx="27.6545" ry="27.6545"/>
        <text text-anchor="middle" x="27.5772" y="-23.7772">yes</text>
        </g>
        <g class="m-node m-flat">
        <title>no</title>
        <ellipse cx="134.9031" cy="-27.5772" rx="25" ry="25"/>
        <text text-anchor="middle" x="134.9031" y="-23.7772">no</text>
        </g>
        <g class="m-edge">
        <title>yes&#45;&gt;no</title>
        <path d="M55.2163,-27.5772C68.8104,-27.5772 85.3444,-27.5772 99.8205,-27.5772"/>
        <polygon points="99.9261,-31.0773 109.9261,-27.5772 99.9261,-24.0773 99.9261,-31.0773"/>
        <text text-anchor="middle" x="82.6543" y="-32.7772">no</text>
        </g>
        <g class="m-edge m-dim">
        <title>no&#45;&gt;no</title>
        <path d="M125.3459,-50.4471C124.3033,-61.0564 127.489,-70.3259 134.9031,-70.3259 139.7685,-70.3259 142.813,-66.3338 144.0365,-60.5909"/>
        <polygon points="147.5398,-60.5845 144.4603,-50.4471 140.5459,-60.2923 147.5398,-60.5845"/>
        <text text-anchor="middle" x="134.9031" y="-75.5259">no</text>
        </g>
        </g>
        </svg>
        <figcaption>Impenetrable logic</figcaption>
        </figure>

`Padding and floating`_
=======================

Similarly to `typography elements <{filename}/css/typography.rst#padding>`_;
blocks, notes, frames, tables, images, figures, image grids, code and math
blocks and code figures have :css:`1rem` padding on the bottom, except when
they are the last element, to avoid excessive spacing. The list special casing
and ability to disable the padding using :css:`.m-nopadb` applies here as well.

Components that appear directly in a column that's :css:`m-container-inflatable`
or directly inside any nested :html:`<section>` are outdented to preserve a
straight line of text alignment on the sides. You can spot that on this page
--- look how notes and figures have their background outside. This also makes
narrow layouts better readable, as the component visuals don't cut into
precious screen width.

All components support the `floating classes <{filename}/css/grid.rst#floating-around>`_ from the grid system, however having floating elements *inside* the
components is not supported. Floating elements also preserve the inflatable
behavior described above.

`Responsive utilities`_
=======================

If you have some element that will certainly overflow on smaller screen sizes
(such as wide table or image that can't be scaled), wrap it in a
:css:`.m-scroll`. This will put a horizontal scrollbar under in case the
element overflows.

There's also :css:`.m-fullwidth` that will make your element always occupy 100%
of the parent element width. Useful for tables or images.
