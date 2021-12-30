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

Writing reST content
####################

:alias: /pelican/writing-content/index.html
:breadcrumb: {filename}/themes.rst Themes
:footer:
    .. note-dim::
        :class: m-text-center

        `Themes <{filename}/themes.rst>`_ | `Pelican » <{filename}/themes/pelican.rst>`_

.. role:: html(code)
    :language: html
.. role:: rst(code)
    :language: rst

While the official `reStructuredText <http://docutils.sourceforge.net/rst.html>`_
documentation provides an extensive syntax guide, the info is scattered across
many pages. The following page will try to explain the basics and mention a
bunch of tricks that might not be immediately obvious.

.. contents::
    :class: m-block m-default

`Basic syntax`_
===============

:abbr:`reST <reStructuredText>` is at its core optimized for fast content
authoring --- the text is easily human-readable, with no excessive markup,
often-used formatting is kept simple and everything is extensible to allow
intuitive usage in many use cases. The basic rules are not far from Markdown:

-   Headers are made by underlining a line of text with some non-alphabetic
    character. Length of the underline has to be same as length of the title.
    Using a different character will create a next level heading, there's no
    rule which character means what (but following some consistent rules across
    all your content is encouraged).
-   Paragraphs are separated from each other by a blank line. Consecutive lines
    of text are concatenated together using spaces, ignoring the newlines. See
    the `Basic block elements`_ section below for more options.
-   Multiple consecutive whitespace characters are rendered as a single space
    (but that's largely due to how HTML behaves).
-   Everything is escaped when converting to HTML, so inline HTML will appear
    as-is, unlike Markdown.
-   As with Python, indentation matters in :abbr:`reST <reStructuredText>`.
    Differently indented lines of text are usually separated via a newline, but exceptions to this rule exist.
-   Lines starting with :rst:`..` are treated as comment together with
    following indented lines as long as the indentation level is not less than
    indentation of the first line.

Example:

.. code:: rst

    Heading 1
    #########

    First paragraph with a bunch of text. Some more text in that paragraph. You can
    wrap your paragraph nicely to fit on your editor screen, the lines will be
    concatenated together with a space in between.

    Second paragraph of text. Each of these paragraphs is rendered as a HTML <p>
    character. The main document heading is not part of the content, but rather an
    implicit document title metadata.

    Heading 2
    =========

    The above heading is using a different underline character so it will be
    rendered as <h2> in the output. Also, the whole section (heading + content
    until the next heading) is rendered as HTML5 <section> element.

    Heading 3
    ---------

    Another character used, another level of headings.

    .. a comment, which is simply ignored when producing HTML output.
        This is still a comment.

        This as well.
            and
            this
        also

    This is not a comment anymore.

    Heading 2
    =========

    Going back to already used character, reST remembers for which heading
    level it was used the first time, so it goes back to <h2>.

There's more information about
`basic syntax rules <http://docutils.sourceforge.net/docs/ref/rst/restructuredtext.html#quick-syntax-overview>`_
in the official :abbr:`reST <reStructuredText>` documentation.

`Metadata fields`_
-------------------

Each document can have a bunch of metadata fields that are not rendered as part
of the main document content. Example of these is an article summary, date or
overriding where given page is saved. Metadata field starts with :rst:`:name:`,
where *name* is field name. After the colon there is a space and field
contents. Consecutive indented lines are treated as part of the same field.

.. code:: rst

    An article
    ##########

    :date: 2017-10-11
    :author: Vladimír Vondruš
    :summary: Article summary. Because article summary can be quite long, let's
        wrap it on multiple lines.

    Article content starts here.

See the `Pelican documentation <https://docs.getpelican.com/en/stable/content.html>`_
for details about recognized fields and how various metadata can be also
automatically extracted from the filesystem. The
`m.css Pelican theme <{filename}/themes/pelican.rst>`_ recognizes a few more
fields.

`Directives`_
-------------

Special block elements (for example to include an image) are called
*directives*, are introduced by a line starting with :rst:`.. name::`, where
*name* is directive name, after the :rst:`::` there can be optional positional
arguments. After that there can be an optional set of named directive options
(indented lines starting with :rst:`:name:` where *name* is option name) and
after that optional directive content, also indented. Unindenting will escape
the directive. Directives can be nested and it's possible to provide
user-defined directives via plugins.

.. note-warning::

    Note that it's possible to use headings only as top-level elements, *not*
    inside any directive or other block element.

Example and corresponding output, note the indentation:

.. code-figure::

    .. code:: rst

        Paragraph with text.

        .. block-info:: Info block title

            Info block content.

            .. figure:: ship.jpg
                :alt: Image alt text

                Figure title

                Figure content.

                .. math::

                    A = \pi r^2

                This is again figure content.

            This is again info block content.

        And this is another paragraph of text, outside of the info block.

    Paragraph with text.

    .. block-info:: Info block title

        Info block content.

        .. figure:: {static}/static/ship-small.jpg
            :alt: Image alt text

            Figure title

            Figure content.

            .. math::

                A = \pi r^2

            This is again figure content.

        This is again info block content.

    And this is another paragraph of text, outside of the info block.

.. note-info::

    Please note that the above example code uses some directives provided by
    `m.css reST plugins <{filename}/plugins.rst>`_ that are not builtin in
    the :abbr:`reST <reStructuredText>` parser itself.

`Interpreted text roles`_
-------------------------

While directives are for block elements, interpreted text roles are for inline
elements. They are part of a paragraph and are in form of :rst:`:name:\`contents\``,
where *name* is role name and *contents* are role contents. The role has to be
separated with non-alphanumeric character from the surroundings; if you need
to avoid the space, escape it with ``\``; similarly with the ````` character,
if you need to use it inside. Unlike directives, roles can't be nested. Roles
are also extensible via plugins.

Roles, like directives, also have options, but the only way to use them is to
define a new role based off the original one with the options you need. Use
the :rst:`.. role::` directive like in the following snippet, where *original*
is optional name of the original role to derive from, *new* is the name of new
role and follows a list of options:

.. code:: rst

    .. role:: new(original)
        :option1: value1
        :option2: value2

Example and a corresponding output:

.. code-figure::

    .. code:: rst

        .. role:: red
            :class: m-text m-danger

        A text paragraph with :emphasis:`emphasised text`, :strong:`strong text`
        and :literal:`typewriter`\ y text. :red:`This text is red.`

    .. role:: red
        :class: m-text m-danger

    A text paragraph with :emphasis:`emphasised text`, :strong:`strong text`
    and :literal:`typewriter`\ y text. :red:`This text is red.`

.. note-success::

    Don't worry, there are less verbose ways to achieve the above formatting.
    Read about `basic inline elements below <#basic-inline-elements>`_.

`Basic block elements`_
=======================

Besides headings and simple paragraphs, there are more block elements like
quotes, literal blocks, tables etc. with implicit syntax. Sometimes you might
want to separate two indented blocks, use a blank line containing just :rst:`..`
to achieve that. Like directives, block elements are also nestable, so you can
have lists inside quotes and the like.

.. code-figure::

    .. code:: rst

        | Line block
        | will
        | preserve the newlines

            A quote is simply an indented block.

        ..

            A different quote.

        ::

            Literal block is itroduced with ::, which can be even part of previous
            paragraph (in which case it's reduced to a single colon).

        ========= ============
        Heading 1 Heading 2
        ========= ============
        Cell 1    Table cell 2
        Row 2     Row 2 cell 2
        Row 3     Row 3 cell 3
        ========= ============

        -   An unordered list
        -   Another item

            1.  Sub-list, ordered
            2.  Another item
            3.  Note that the sub-list is separated by blank lines

        -   Third item of the top-level list

        Term 1
            Description
        Term 2
            Description of term 2

    .. class:: m-noindent

    | Line block
    | will
    | preserve the newlines

        A quote is simply an indented block.

    ..

        A different quote.

    ::

        Literal block is itroduced with ::, which can be even the ending of
        previous paragraph (in which case it's reduced to a single colon).

    .. class:: m-table

    ========= ============
    Heading 1 Heading 2
    ========= ============
    Cell 1    Cell 2
    Row 2     Row 2 cell 2
    Row 3     Cell 3
    ========= ============

    -   An unordered list
    -   Another item

        1.  Sub-list, ordered
        2.  Another item
        3.  Note that the sub-list is separated by blank lines

    -   Third item of the top-level list

    .. class:: m-diary

    Term 1
        Description
    Term 2
        Description of term 2

The official :abbr:`reST <reStructuredText>` documentation has more detailed
info about `block elements <http://docutils.sourceforge.net/docs/ref/rst/restructuredtext.html#body-elements>`_.

`Basic inline elements`_
========================

The :rst:`:emphasis:` role can be written shorter by wrapping the content in
``*``, :rst:`:strong:` using ``**`` and :rst:`:literal:` with ``````. A single
backtick :rst:`\`` is reserved and you can redefine to whatever you need in
given scope using the `default-role <http://docutils.sourceforge.net/docs/ref/rst/directives.html#default-role>`_
directive.

External links are written using :rst:`\`title <URL>\`_`, where *title* is link
text and *URL* is the link destination. The link title can be then used again
to link to the same URL as simply :rst:`\`title\`_`. Note that specifying two
links with the same title and different URLs is an error --- if you need that,
use anonymous links that are in a form of :rst:`\`title <URL>\`__` (two
underscores after).

Internal links: every heading can be linked to using :rst:`\`heading\`_`, where
*heading* is the heading text, moreover the heading itself can be wrapped in
backticks and an underscore to make it a clickable link pointing to itself.
Arbitrary anchors inside the text flow that can be linked to later can be
created using the :rst:`.. _anchor:` syntax.

.. code-figure::

    .. code:: rst

        .. _an anchor:

        An *emphasised text*, **strong text** and a ``literal``. Link to
        `Google <https://google.com>`_, `the heading below <#a-heading>`_ or just an
        URL as-is: https://mcss.mosra.cz/.

        `A heading`_
        ============

        Repeated link to `Google`_. Anonymous links that share the same titles
        `here <http://blog.mosra.cz>`__ and `here <https://magnum.graphics/>`__.
        Link to `an anchor`_ above.

    .. _an anchor:

    An *emphasised text*, **strong text** and a ``literal``. Link to
    `Google <https://google.com>`_, `the heading below <#a-heading>`_ or just
    an URL as-is: https://mcss.mosra.cz/.

    .. raw:: html

        <section id="a-heading">
        <h2><a href="#a-heading">A heading</a></h2>

    Repeated link to `Google`_. Anonymous links that share the same titles
    `here <http://blog.mosra.cz>`__ and `here <https://magnum.graphics/>`__.
    Link to `an anchor`_ above.

    .. raw:: html

        </section>

There are some special features in Pelican for easier linking to internal
content, be sure to `check out the documentation <https://docs.getpelican.com/en/stable/content.html#linking-to-internal-content>`_.
The :abbr:`reST <reStructuredText>` documentation also offers more detailed
info about `inline markup <http://docutils.sourceforge.net/docs/ref/rst/restructuredtext.html#inline-markup>`_.

`Essential directives`_
=======================

-   A `container <http://docutils.sourceforge.net/docs/ref/rst/directives.html#container>`_
    directive will just put a :html:`<div>` around its contents and its
    arguments will be put as CSS classes to that element.
-   A `class <http://docutils.sourceforge.net/docs/ref/rst/directives.html#class>`_
    directive will put its argument as CSS class to the immediately following
    element. Besides that, most of the directives also accept a :rst:`:class:`
    option that does the same.
-   Sometimes you need to put raw HTML code onto your page (for example to
    embed a third-party widget). Use the `raw <http://docutils.sourceforge.net/docs/ref/rst/directives.html#raw-data-pass-through>`_
    directive to achieve that.
-   For including larger portions of raw HTML code or bigger code snippets
    there's the `include <http://docutils.sourceforge.net/docs/ref/rst/directives.html#including-an-external-document-fragment>`_
    directive, you can also specify just a portion of the file either by line
    numbers or text match.
-   The `contents <http://docutils.sourceforge.net/docs/ref/rst/directives.html#contents>`_
    directive will automatically make a Table of Contents list out of headings
    in your document. Very useful for navigation in large pages and articles.

For stuff like images, figures, code blocks, math listing etc., m.css provides
`various plugins <{filename}/plugins.rst>`_ that do it better than the builtin
way. Head over to the official :abbr:`reST <reStructuredText>` documentation
for `more info about builtin directives <http://docutils.sourceforge.net/docs/ref/rst/directives.html>`_.

`Essential interpreted text roles`_
===================================

-   Besides the already mentioned roles, there's also a
    `sup <http://docutils.sourceforge.net/docs/ref/rst/roles.html#superscript>`_
    and `sub <http://docutils.sourceforge.net/docs/ref/rst/roles.html#subscript>`_
    role for superscript and subscript text.
-   It's also possible to put raw HTML code inline by deriving from the
    `raw <http://docutils.sourceforge.net/docs/ref/rst/roles.html#raw>`__ role.

Again, m.css provides `various plugins`_ that allow you to have inline
code, math, GitHub and Doxygen links and much more. Head over to the official
:abbr:`reST <reStructuredText>` documentation for
`more info about builtin roles <http://docutils.sourceforge.net/docs/ref/rst/roles.html>`_.
