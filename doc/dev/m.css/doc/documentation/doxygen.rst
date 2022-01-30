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

Doxygen C++ theme
#################

:alias: /doxygen/index.html
:breadcrumb: {filename}/documentation.rst Doc generators
:summary: A modern, mobile-friendly drop-in replacement for the stock Doxygen
    HTML output with a first-class search functionality
:footer:
    .. note-dim::
        :class: m-text-center

        `Doc generators <{filename}/documentation.rst>`_ | `Python doc theme » <{filename}/documentation/python.rst>`_

.. role:: cpp(code)
    :language: cpp
.. role:: css(code)
    :language: css
.. role:: html(code)
    :language: html
.. role:: ini(code)
    :language: ini
.. role:: jinja(code)
    :language: jinja
.. role:: js(code)
    :language: js
.. role:: py(code)
    :language: py
.. role:: sh(code)
    :language: sh

A modern, mobile-friendly drop-in replacement for the stock
`Doxygen <https://www.doxygen.org>`_ HTML output, with a first-class search
functionality. Generated from Doxygen-produced XML files, filenames and URLs
are fully compatible with the stock output to avoid broken links once you
switch.

.. button-success:: https://doc.magnum.graphics

    Live demo

    doc.magnum.graphics

.. contents::
    :class: m-block m-default

`Basic usage`_
==============

.. note-info::

    The script requires at least Doxygen 1.8.14, but preferably version 1.8.15
    and newer. Some features depend on newer versions, in that case the
    documentation mentions which version contains the support.

The base is contained in a single Python script and related style/template
files, for advanced features such as math rendering it'll make use of internals
of some `m.css plugins <{filename}/plugins.rst>`_. Clone
:gh:`the m.css GitHub repository <mosra/m.css$master/documentation>` and look
into the ``documentation/`` directory:

.. code:: sh

    git clone git://github.com/mosra/m.css
    cd m.css/documentation

The script requires Python 3.6, depends on `Jinja2 <http://jinja.pocoo.org/>`_
for templating and `Pygments <http://pygments.org/>`_ for code block
highlighting. You can install the dependencies via ``pip`` or your distribution
package manager:

.. code:: sh

    # You may need sudo here
    pip3 install jinja2 Pygments

If your documentation includes math formulas, in addition you need some LaTeX
distribution installed. Use your distribution package manager, for example on
Ubuntu:

.. code:: sh

    sudo apt install \
        texlive-base \
        texlive-latex-extra \
        texlive-fonts-extra \
        texlive-fonts-recommended

.. note-success::

    This tool makes use of the ``latex2svg.py`` utility from :gh:`tuxu/latex2svg`,
    © 2017 `Tino Wagner <http://www.tinowagner.com/>`_, licensed under
    :gh:`MIT <tuxu/latex2svg$master/LICENSE.md>`.

Now, in order to preserve your original Doxygen configuration, create a new
``Doxyfile-mcss`` file next to your original ``Doxyfile`` and put the following
inside:

.. code:: ini

    @INCLUDE                = Doxyfile
    GENERATE_HTML           = NO
    GENERATE_XML            = YES
    XML_PROGRAMLISTING      = NO

This will derive the configuration from the original ``Doxyfile``, disables
builtin Doxygen HTML output and enables XML output instead, with some unneeded
features disabled for faster processing. Now run ``doxygen.py`` and point it
to your ``Doxyfile-mcss``:

.. code:: sh

    ./doxygen.py path/to/your/Doxyfile-mcss

It will run ``doxygen`` to generate the XML output, processes it and generates
the HTML output in the configured output directory. After the script is done,
just open generated ``index.html`` to see the result.

If you see something unexpected or not see something expected, check the
`Troubleshooting`_ section below.

`Features`_
===========

In addition to features `shared by all doc generators <{filename}/documentation.rst#features>`_:

-   Modern, valid, mobile-friendly HTML5 markup without table layouts
-   URLs fully compatible with stock Doxygen HTML output to preserve existing
    links
-   Focused on presenting the actual written documentation while reducing
    questionable auto-generated content

`Important differences to stock HTML output`_
---------------------------------------------

-   Detailed description is put first and foremost on a page, *before* the
    member listing
-   Files, directories and symbols that don't have any documentation are not
    present in the output at all. This is done in order to encourage good
    documentation practices --- having the output consist of an actual
    human-written documentation instead of just autogenerated lists. If you
    *really* want to have them included in the output, you can enable them
    using a default-to-off :py:`SHOW_UNDOCUMENTED` option in ``conf.py`` (or
    :ini:`M_SHOW_UNDOCUMENTED` in the ``Doxyfile``), but there are some
    tradeoffs. See `Showing undocumented symbols and files`_ for more
    information.
-   Table of contents is generated for compound references as well, containing
    all sections of detailed description together with anchors to member
    listings
-   Private members and anonymous namespaces are always ignored, however
    private virtual functions are listed in case they are documented.
    See `Private virtual functions`_ for more information.
-   Inner classes are listed in the public/protected type sections instead of
    being listed in a separate section ignoring their public/private status
-   Class references contain also their template specification on the linked
    page
-   Function signatures don't contain :cpp:`constexpr` and :cpp:`noexcept`
    anymore. These keywords are instead added as flags to the function
    description together with :cpp:`virtual`\ ness, :cpp:`explicit`\ ity and
    :cpp:`override` / :cpp:`final` status. On the other hand, important
    properties like :cpp:`static`, :cpp:`const` and r-value overloads *are*
    part of function signature.
-   For better visual alignment, function listing is done using the C++11
    trailing return type (:cpp:`auto` in front) and typedef listing is done
    with :cpp:`using`). However, the detailed documentation is kept in the
    original form.
-   Function and macro parameters and enum values are vertically aligned in
    the member listing for better readability
-   Default class template parameters are not needlessly repeated in each
    member detailed docs
-   Deprecation markers are propagated to member and compound listing pages and
    search results; :cpp:`delete`\ d functions are marked in search as well
-   Information about which file to :cpp:`#include` for given symbol is
    provided also for namespaces, free and related functions, enums, typedefs
    and variables. See `Include files`_ for more information.

`Intentionally unsupported features`_
-------------------------------------

.. note-danger:: Warning: opinions

    This list presents my opinions. Not everybody likes my opinions.

Features that I don't see a point in because they just artificially inflate the
amount of generated content for no added value.

-   Class hierarchy graphs are ignored (it only inflates the documentation with
    little added value)
-   Alphabetical list of symbols and alphabetical list of all members of a
    class is not created (the API *should be* organized in a way that makes
    this unnecessary, there's also search for this)
-   Verbatim listing of parsed headers, "Includes" and "Included By" lists are
    not present (use your IDE or GitHub instead)
-   Initializers of defines and variables are unconditionally ignored (one can
    always look in the sources, if really needed)
-   No section with list of examples or linking from function/class
    documentation to related example code (he example code should be
    accompanied with corresponding tutorial page instead)
-   :cpp:`inline` functions are not marked as such (I see it as an unimportant
    implementation detail)
-   The :ini:`CREATE_SUBDIRS` Doxyfile option is not supported. This option
    causes Doxygen to scatter the XML files across numerous subdirectories to
    work around limits of ancient filesystems. Implementing support for this
    option would be too much effort for too little gain and so m.css simply
    aborts if it discovers this option being enabled. Set it back to ``NO`` it
    in your ``Doxyfile-mcss`` override.
-   The :ini:`SHOW_NAMESPACES = NO` Doxyfile option is not supported as the
    theme provides a much more flexible configuration of what's shown in the
    top navbar. See `Navbar links`_ for more information.

`Not yet implemented features`_
-------------------------------

-   Clickable symbols in code snippets. Doxygen has quite a lot of false
    positives while a lot of symbols stay unmatched. I need to find a way
    around that.
-   Documented friend classes, structs and unions. Doxygen is unable to
    cross-link the declarations with the definitions.
-   Proper scoping for friend and related functions/classes/variables etc.
    Doxygen doesn't provide any namespace scoping for these and at the moment
    I have no way to deduct that information.
-   APIs listed in file documentation pages lose all namespace information (and
    when it appears in search, it's prefixed with the file name instead of the
    namespace). This is due to several bugs on Doxygen side, which still need
    to be patched (or worked around).

`Configuration`_
================

The script takes a part of the configuration from the ``Doxyfile`` itself,
(ab)using the following builtin options:

.. class:: m-table m-fullwidth

=============================== ===============================================
Variable                        Description
=============================== ===============================================
:ini:`@INCLUDE`                 Includes in ``Doxyfile``\ s are supported
:ini:`PROJECT_NAME`             Rendered in top navbar, footer fine print and
                                page title
:ini:`PROJECT_BRIEF`            If set, appended in a thinner font to
                                :ini:`PROJECT_NAME`
:ini:`PROJECT_LOGO`             URL of an image file to use as a log in the top
                                navbar. Default is none.
:ini:`OUTPUT_DIRECTORY`         Used to discover where Doxygen generates the
                                files
:ini:`XML_OUTPUT`               Used to discover where Doxygen puts the
                                generated XML
:ini:`HTML_OUTPUT`              The output will be written here
:ini:`TAGFILES`                 Used to discover what base URL to prepend to
                                external references
:ini:`DOT_FONTNAME`             Font name to use for ``@dot`` and ``@dotfile``
                                commands. To ensure consistent look with the
                                default m.css themes, set it to
                                ``Source Sans Pro``. Doxygen default is
                                ``Helvetica``.
:ini:`DOT_FONTSIZE`             Font size to use for ``@dot`` and ``@dotfile``
                                commands. To ensure consistent look with the
                                default m.css themes, set it to ``16``.
                                Doxygen default is ``10``.
:ini:`SHOW_INCLUDE_FILES`       Whether to show corresponding :cpp:`#include`
                                file for classes, namespaces and namespace
                                members. Originally :ini:`SHOW_INCLUDE_FILES`
                                is meant to be for "a list of the files that
                                are included by a file in the documentation of
                                that file" but that kind of information is
                                glaringly useless in every imaginable way and
                                thus the theme is reusing it for something
                                actually useful. Doxygen default is ``YES``.
=============================== ===============================================

On top of the above, the script can take additional options in a way consistent
with the `Python documentation generator <{filename}python.rst#configuration>`_.
The recommended and most flexible way is to create a ``conf.py`` file
referencing the original Doxyfile:

.. code:: py

    DOXYFILE = 'Doxyfile-mcss'

    # additional options from the table below

and then pass that file to the script, instead of the original Doxyfile:

.. code:: sh

    ./doxygen.py path/to/your/conf.py

.. class:: m-table m-fullwidth

=================================== ===========================================
Variable                            Description
=================================== ===========================================
:py:`MAIN_PROJECT_URL: str`         If set and :ini:`PROJECT_BRIEF` is also
                                    set, then :ini:`PROJECT_NAME` in the top
                                    navbar will link to this URL and
                                    :ini:`PROJECT_BRIEF` to the documentation
                                    main page, similarly as `shown here <{filename}/css/page-layout.rst#link-back-to-main-site-from-a-subsite>`_.
:py:`THEME_COLOR: str`              Color for :html:`<meta name="theme-color" />`,
                                    corresponding to the CSS style. If not set,
                                    no :html:`<meta>` tag is rendered. See
                                    `Theme selection`_ for more information.
:py:`FAVICON: str`                  Favicon URL, used to populate
                                    :html:`<link rel="icon" />`. If empty, no
                                    :html:`<link>` tag is rendered. Relative
                                    paths are searched relative to the
                                    :abbr:`config file <Doxyfile or conf.py>`
                                    base dir and to the ``doxygen.py`` script
                                    dir as a fallback. See `Theme selection`_
                                    for more information.
:py:`STYLESHEETS: List[str]`        List of CSS files to include. Relative
                                    paths are searched relative to the
                                    :abbr:`config file <Doxyfile or conf.py>`
                                    base dir and to the ``doxygen.py`` script
                                    dir as a fallback. See `Theme selection`_
                                    for more information.
:py:`HTML_HEADER: str`              HTML code to put at the end of the
                                    :html:`<head>` element. Useful for linking
                                    arbitrary JavaScript code or, for example,
                                    adding :html:`<link>` CSS stylesheets with
                                    additional properties and IDs that are
                                    otherwise not possible with just
                                    :ini:`HTML_EXTRA_STYLESHEET`
:py:`EXTRA_FILES: List[str]`        List of extra files to copy (for example
                                    additional CSS files that are :css:`@import`\ ed
                                    from the primary one). Relative paths are
                                    searched relative to the
                                    :abbr:`config file <Doxyfile or conf.py>`
                                    base dir and to the ``doxygen.py`` script
                                    dir as a fallback.
:py:`LINKS_NAVBAR1: List[Any]`      Left navbar column links. See
                                    `Navbar links`_ for more information.
:py:`LINKS_NAVBAR2: List[Any]`      Right navbar column links. See
                                    `Navbar links`_ for more information.
:py:`PAGE_HEADER: str`              HTML code to put at the top of every page.
                                    Useful for example to link to different
                                    versions of the same documentation. The
                                    ``{filename}`` placeholder is replaced with
                                    current file name.
:py:`FINE_PRINT: str`               HTML code to put into the footer. If not
                                    set, a default generic text is used. If
                                    empty, no footer is rendered at all. The
                                    ``{doxygen_version}`` placeholder is
                                    replaced with Doxygen version that
                                    generated the input XML files.
:py:`CLASS_INDEX_EXPAND_LEVELS`     How many levels of the class tree to
                                    expand. :py:`0` means only the top-level
                                    symbols are shown. If not set, :py:`1` is
                                    used.
:py:`CLASS_INDEX_EXPAND_INNER`      Whether to expand inner types (e.g. a class
                                    inside a class) in the symbol tree. If not
                                    set, :py:`False` is used.
:py:`FILE_INDEX_EXPAND_LEVELS`      How many levels of the file tree to expand.
                                    :py:`0` means only the top-level dirs/files
                                    are shown. If not set, :py:`1` is used.
:py:`SEARCH_DISABLED: bool`         Disable search functionality. If this
                                    option is set, no search data is compiled
                                    and the rendered HTML does not contain any
                                    search-related UI or support. If not set,
                                    :py:`False` is used.
:py:`SEARCH_DOWNLOAD_BINARY`        Download search data as a binary to save
                                    bandwidth and initial processing time. If
                                    not set, :py:`False` is used. See
                                    `Search options`_ for more information.
:py:`SEARCH_HELP: str`              HTML code to display as help text on empty
                                    search popup. If not set, a default message
                                    is used. Has effect only if
                                    :py:`SEARCH_DISABLED` is not :py:`True`.
:py:`SEARCH_BASE_URL: str`          Base URL for OpenSearch-based search engine
                                    suggestions for web browsers. See
                                    `Search options`_ for more information. Has
                                    effect only if :py:`SEARCH_DISABLED` is
                                    not :py:`True`.
:py:`SEARCH_EXTERNAL_URL: str`      URL for external search. The ``{query}``
                                    placeholder is replaced with urlencoded
                                    search string. If not set, no external
                                    search is offered. See `Search options`_
                                    for more information. Has effect only if
                                    :py:`SEARCH_DISABLED` is not :py:`True`.
:py:`VERSION_LABELS: bool`          Show the ``@since`` annotation as labels
                                    visible in entry listing and detailed docs.
                                    Defaults to :py:`False`, see `Version labels`_
                                    for more information.
:py:`SHOW_UNDOCUMENTED: bool`       Include undocumented symbols, files and
                                    directories in the output. If not set,
                                    :py:`False` is used. See `Showing undocumented symbols and files`_
                                    for more information.
:py:`M_MATH_CACHE_FILE`             File to cache rendered math formulas. If
                                    not set, ``m.math.cache`` file in the
                                    output directory is used. Equivalent to an
                                    option of the same name in the
                                    `m.math plugin <{filename}/plugins/math-and-code.rst#math>`.
:py:`M_CODE_FILTERS_PRE: Dict`      Filters to apply before a code snippet is
                                    rendered. Equivalent to an option of the
                                    same name in the `m.code plugin <{filename}/plugins/math-and-code.rst#filters>`.
                                    Note that due to the limitations of Doxygen
                                    markup, named filters are not supported.
:py:`M_CODE_FILTERS_POST: Dict`     Filters to apply after a code snippet is
                                    rendered. Equivalent to an option of the
                                    same name in the `m.code plugin <{filename}/plugins/math-and-code.rst#filters>`.
                                    Note that due to the limitations of Doxygen
                                    markup, named filters are not supported.
=================================== ===========================================

Note that namespace, directory and page lists are always fully expanded as
these are not expected to be excessively large.

.. block-warning:: Legacy configuration through extra Doxyfile options

    Originally, the above options were parsed from the Doxyfile as well, but
    the Doxyfile format limited the flexibility quite a lot. These are still
    supported for backwards compatibility and map to the above options as shown
    below. Integer and string values are parsed as-is, list items are parsed
    one item per line with ``\`` for line continuations and boolean values have
    to be either ``YES`` or ``NO``.

    Doxygen complains on unknown options, so as a workaround one can add them
    prefixed with ``##!``. Line continuations are supported too, using ``##!``
    ensures that the options also survive Doxyfile upgrades using
    ``doxygen -u`` (which is not the case when the options would be specified
    directly):

    .. code:: ini

        ##! M_LINKS_NAVBAR1 = pages \
        ##!                   modules

    .. class:: m-table m-fullwidth

    =================================== =======================================
    Legacy ``Doxyfile`` variable        Corresponding ``conf.py`` variable
    =================================== =======================================
    :ini:`HTML_EXTRA_STYLESHEET`        :py:`STYLESHEETS`
    :ini:`HTML_EXTRA_FILES`             :py:`EXTRA_FILE`
    :ini:`M_THEME_COLOR`                :py:`THEME_COLOR`
    :ini:`M_FAVICON`                    :py:`FAVICON`
    :ini:`M_LINKS_NAVBAR1`              :py:`LINKS_NAVBAR1`. The syntax is
                                        different in each case, see
                                        `Navbar links`_ for more information.
    :ini:`M_LINKS_NAVBAR2`              :py:`LINKS_NAVBAR2`. The syntax is
                                        different in each case, see
                                        `Navbar links`_ for more information.
    :ini:`M_MAIN_PROJECT_URL`           :py:`MAIN_PROJECT_URL`
    :ini:`M_HTML_HEADER`                :py:`HTML_HEADER`
    :ini:`M_PAGE_HEADER`                :py:`PAGE_HEADER`
    :ini:`M_PAGE_FINE_PRINT`            :py:`FINE_PRINT`
    :ini:`M_CLASS_TREE_EXPAND_LEVELS`   :py:`CLASS_INDEX_EXPAND_LEVELS`
    :ini:`M_FILE_TREE_EXPAND_LEVELS`    :py:`FILE_INDEX_EXPAND_LEVELS`
    :ini:`M_EXPAND_INNER_TYPES`         :py:`CLASS_INDEX_EXPAND_INNER`
    :ini:`M_MATH_CACHE_FILE`            :py:`M_MATH_CACHE_FILE`
    :ini:`M_SEARCH_DISABLED`            :py:`SEARCH_DISABLED`
    :ini:`M_SEARCH_DOWNLOAD_BINARY`     :py:`SEARCH_DOWNLOAD_BINARY`
    :ini:`M_SEARCH_HELP`                :py:`SEARCH_HELP`
    :ini:`M_SEARCH_BASE_URL`            :py:`SEARCH_BASE_URL`
    :ini:`M_SEARCH_EXTERNAL_URL`        :py:`SEARCH_EXTERNAL_URL`
    :ini:`M_VERSION_LABELS`             :py:`VERSION_LABELS`
    :ini:`M_SHOW_UNDOCUMENTED`          :py:`SHOW_UNDOCUMENTED`
    =================================== =======================================

`Theme selection`_
------------------

By default, the `dark m.css theme <{filename}/css/themes.rst#dark>`_ together
with documentation-theme-specific additions is used, which corresponds to the
following configuration:

.. code:: py
    :class: m-console-wrap

    STYLESHEETS = [
        'https://fonts.googleapis.com/css?family=Source+Sans+Pro:400,400i,600,600i%7CSource+Code+Pro:400,400i,600',
        '../css/m-dark+documentation.compiled.css'
    ]
    THEME_COLOR = '#22272e'
    FAVICON = 'favicon-dark.png'

If you have a site already using the ``m-dark.compiled.css`` file, there's
another file called ``m-dark.documentation.compiled.css``, which contains just
the documentation-theme-specific additions so you can reuse the already cached
``m-dark.compiled.css`` file from your main site:

.. code:: py
    :class: m-console-wrap

    STYLESHEETS = [
        'https://fonts.googleapis.com/css?family=Source+Sans+Pro:400,400i,600,600i%7CSource+Code+Pro:400,400i,600',
        '../css/m-dark.compiled.css',
        '../css/m-dark.documentation.compiled.css'
    ]
    THEME_COLOR = '#22272e'
    FAVICON = 'favicon-dark.png'

If you prefer the `light m.css theme <{filename}/css/themes.rst#light>`_
instead, use the following configuration (and, similarly, you can use
``m-light.compiled.css`` together with ``m-light.documentation.compiled-css``
in place of ``m-light+documentation.compiled.css``:

.. code:: py
    :class: m-console-wrap

    STYLESHEETS = [
        'https://fonts.googleapis.com/css?family=Libre+Baskerville:400,400i,700,700i%7CSource+Code+Pro:400,400i,600',
        '../css/m-light+documentation.compiled.css'
    ]
    THEME_COLOR = '#cb4b16'
    FAVICON = 'favicon-light.png'

See the `CSS files`_ section below for more information about customizing the
CSS files.

`Navbar links`_
---------------

The :py:`LINKS_NAVBAR1` and :py:`LINKS_NAVBAR2` options define which links are
shown on the top navbar, split into left and right column on small screen
sizes. These options take a list of :py:`(title, path, sub)` tuples ---
``title`` is the link title; ``path`` is either one of the :py:`'pages'`,
:py:`'modules'`, :py:`'namespaces'`, :py:`'annotated'`, :py:`'files'` IDs or a
compound ID; and ``sub`` is an optional submenu, containing :py:`(title, path)`
with ``path`` being interpreted the same way.

By default the variables are defined like following --- two items in the left
column, two items in the right columns, with no submenus:

.. code:: py

    LINKS_NAVBAR1 = [
        ("Pages", 'pages', []),
        ("Namespaces", 'namespaces', [])
    ]
    'LINKS_NAVBAR2' = [
        ("Classes", 'annotated', []),
        ("Files", 'files', [])
    ]

.. note-info::

    The theme by default assumes that the project is grouping symbols in
    namespaces. If you use modules (``@addtogroup`` and related commands) and
    you want to show their index in the navbar, add ``modules`` to one of
    the :py:`LINKS_NAVBAR*` options, for example:

    .. code:: py

        LINKS_NAVBAR1 = [
            ("Pages", 'pages', []),
            ("Modules", 'modules', [])
        ]
        LINKS_NAVBAR2 = [
            ("Classes", 'annotated', []),
            ("Files", 'files', [])
        ]

If the title is :py:`None`, it's taken implicitly from the page it links to.
Empty :py:`LINKS_NAVBAR2` will cause the navigation appear in a single column,
setting both empty will cause the navbar links to not be rendered at all.

A menu item is highlighted if a compound with the same ID is the current page
(and similarly for the special ``pages``, ... IDs).

Alternatively, a link can be a plain HTML instead of the first pair of tuple
values, in which case it's put into the navbar as-is. It's not limited to just
a text, but it can contain an image, embedded SVG or anything else. A complex
example including submenus follows --- the first navbar column will have links
to namespaces *Foo*, *Bar* and *Utils* as a sub-items of a top-level
*Namespaces* item and links to two subdirectories as sub-items of the *Files*
item, with title being :py:`None` to have it automatically filled in by
Doxygen. The second column has two top-level items, first linking to the page
index and having a submenu linking to an e-mail address and a ``fine-print``
page and the second linking to a GitHub project page. Note the mangled names,
corresponding to filenames of given compounds generated by Doxygen.

.. code:: py

    LINKS_NAVBAR1 = [
        (None, 'namespaces', [
            (None, 'namespaceFoo'),
            (None, 'namespaceBar'),
            (None, 'namespaceUtils'),
        ]),
        (None, 'files', [
            (None, 'dir_d3b07384d113edec49eaa6238ad5ff00'),
            (None, 'dir_cbd8f7984c654c25512e3d9241ae569f')
        ])
    ]
    LINKS_NAVBAR2 = [
        ("Pages", 'pages', [
            ("<a href=\"mailto:mosra@centrum.cz\">Contact</a>", ),
            ("Fine print", 'fine-print')
        ]),
        ("<a href=\"https://github.com/mosra/m.css\">GitHub</a>", [])
    ]

.. block-warning:: Legacy navbar link specification in the Doxyfile

    For backwards compatibility, the :ini:`M_LINKS_NAVBAR1` and
    :ini:`M_LINKS_NAVBAR2` options in the Doxyfile are recognized. Compared to
    the Python variant above, the encoding is a bit more complicated --- these
    options take a whitespace-separated list of compound IDs and additionally
    the special ``pages``, ``modules``, ``namespaces``, ``annotated``,
    ``files`` IDs. An equivalent to the above default Python config would be
    the following --- titles for the links are always taken implicitly in this
    case:

    .. code:: ini

        M_LINKS_NAVBAR1 = pages namespaces
        M_LINKS_NAVBAR2 = annotated files

    It's possible to specify sub-menu items by enclosing more than one ID in
    quotes. The top-level items then have to be specified each on a single
    line. Example (note the mangled names, corresponding to filenames of given
    compounds generated by Doxygen):

    .. code:: ini

        M_LINKS_NAVBAR1 = \
            "namespaces namespaceFoo namespaceBar namespaceUtils" \
            "files dir_d3b07384d113edec49eaa6238ad5ff00 dir_cbd8f7984c654c25512e3d9241ae569f"

    This will put links to namespaces Foo, Bar and Utils as a sub-items of a
    top-level *Namespaces* item and links to two subdirectories as sub-items of
    the *Files* item.

    For custom links in the navbar it's possible to use HTML code directly,
    both for a top-level item or in a submenu. The item is taken as everything
    from the initial :html:`<a` to the first closing :html:`</a>`. In the
    following snippet, there are two top-level items, first linking to the page
    index and having a submenu linking to an e-mail address and a
    ``fine-print`` page and the second linking to a GitHub project page:

    .. code:: ini

        M_LINKS_NAVBAR2 = \
            "pages <a href=\"mailto:mosra@centrum.cz\">Contact</a> fine-print" \
            "<a href=\"https://github.com/mosra/m.css\">GitHub</a>"

`Search options`_
-----------------

Symbol search is implemented using JavaScript Typed Arrays and does not need
any server-side functionality to perform well --- the client automatically
downloads a tightly packed binary containing search data and performs search
directly on it.

However, due to `restrictions of Chromium-based browsers <https://bugs.chromium.org/p/chromium/issues/detail?id=40787&q=ajax%20local&colspec=ID%20Stars%20Pri%20Area%20Feature%20Type%20Status%20Summary%20Modified%20Owner%20Mstone%20OS>`_,
it's not possible to download data using :js:`XMLHttpRequest` when served from
a local file-system. Because of that, the search defaults to producing a
Base85-encoded representation of the search binary and loading that
asynchronously as a plain JavaScript file. This results in the search data
being 25% larger, but since this is for serving from a local filesystem, it's
not considered a problem. If your docs are accessed through a server (or you
don't need Chrome support), enable the :py:`SEARCH_DOWNLOAD_BINARY` option.

The site can provide search engine metadata using the `OpenSearch <http://www.opensearch.org/>`_
specification. On supported browsers this means you can add the search field to
search engines and search directly from the address bar. To enable search
engine metadata, point :py:`SEARCH_BASE_URL` to base URL of your documentation,
for example:

.. code:: py

    SEARCH_BASE_URL = "https://doc.magnum.graphics/magnum/"

In general, even without the above setting, appending ``?q={query}#search`` to
the URL will directly open the search popup with results for ``{query}``.

.. note-info::

    OpenSearch also makes it possible to have autocompletion and search results
    directly in the browser address bar. However that requires a server-side
    search implementation and is not supported at the moment.

If :py:`SEARCH_EXTERNAL_URL` is specified, full-text search using an external
search engine is offered if nothing is found for given string or if the user
has JavaScript disabled. It's recommended to restrict the search to a
particular domain or add additional keywords to the search query to filter out
irrelevant results. Example, using Google search engine and restricting the
search to a subdomain:

.. code:: py

    SEARCH_EXTERNAL_URL = "https://google.com/search?q=site:doc.magnum.graphics+{query}"

`Showing undocumented symbols and files`_
-----------------------------------------

As noted in `Features`_, the main design decision of the m.css Doxygen theme is
to reduce the amount of useless output. A plain list of undocumented APIs also
counts as useless output, so by default everything that's not documented is not
shown.

In some cases, however, it might be desirable to show undocumented symbols as
well --- for example when converting an existing project from vanilla Doxygen
to m.css, not all APIs might be documented yet and thus the output would be
incomplete. The :py:`SHOW_UNDOCUMENTED` option in ``conf.py`` (or
:ini:`M_SHOW_UNDOCUMENTED` in the ``Doxyfile``) unconditionally makes all
undocumented symbols, files and directories "appear documented". Note, however,
that Doxygen itself doesn't allow to link to undocumented symbols and so even
though the undocumented symbols are present in the output, nothing is able to
reference them, causing very questionable usability of such approach. A
potential "fix" to this is enabling the :ini:`EXTRACT_ALL` Doxyfile option, but
that exposes all symbols, including private and file-local ones --- which, most
probably, is *not* what you want.

If you have namespaces not documented, Doxygen will by put function docs into
file pages --- but it doesn't put them into the XML output, meaning all links
to them will lead nowhere and the functions won't appear in search either.
To fix this, enable the :ini:`XML_NS_MEMB_FILE_SCOPE` Doxyfile option as
described in the `Namespace members in file scope`_ section below; if you
document all namespaces this problem will go away as well.

`Content`_
==========

Brief and detailed description is parsed as-is with the following
modifications:

-   Function parameter documentation, return value documentation, template
    parameter and exception documentation is extracted out of the text flow to
    allow for more flexible styling. It's also reordered to match parameter
    order and warnings are emitted if there are mismatches.
-   To make text content wrap better on narrow screens, :html:`<wbr/>` tags are
    added after ``::`` and ``_`` in long symbols in link titles and after ``/``
    in URLs.

Single-paragraph list items, function parameter description, table cell content
and return value documentation is stripped from the enclosing :html:`<p>` tag
to make the output more compact. If multiple paragraphs are present, nothing is
stripped. In case of lists, they are then rendered in an inflated form.
However, in order to achieve even spacing also with single-paragraph items,
it's needed use some explicit markup. Adding :html:`<p></p>` to a
single-paragraph item will make sure the enclosing :html:`<p>` is not stripped.

.. code-figure::

    .. code:: c++

        /**
        -   A list

            of multiple

            paragraphs.

        -   Another item

            <p></p>

            -   A sub list

                Another paragraph
        */

    .. raw:: html

        <ul>
          <li>
            <p>A list</p>
            <p>of multiple</p>
            <p>paragraphs.</p>
          </li>
          <li>
            <p>Another item</p>
            <ul>
              <li>
                <p>A sub list</p>
                <p>Another paragraph</p>
              </li>
            </ul>
          </li>
        </ul>

`Images and figures`_
---------------------

To match the stock HTML output, images that are marked with ``html`` target are
used. If image name is present, the image is rendered as a figure with caption.
It's possible affect width/height of the image using the ``sizespec`` parameter
(unlike stock Doxygen, which makes use of this field only for LaTeX output and
ignores it for HTML output). The parameter is converted to an inline CSS
:css:`width` or :css:`height` property, so the value has to contain the units
as well:

.. code:: c++

    /**
    @image image.png width=250px
    */

`Dot graphs`_
-------------

Grapviz ``dot`` graphs from the ``@dot`` and ``@dotfile`` commands are rendered
as an inline SVG. Graph name and the ``sizespec`` works equivalently to the
`Images and figures`_. In order to be consistent with the theme, set a matching
font name and size in the `Doxyfile` --- these values match the default dark
theme:

.. code:: ini

    DOT_FONTNAME = Source Sans Pro
    DOT_FONTSIZE = 16.0

See documentation of the `m.dot <{filename}/plugins/plots-and-graphs.rst#graphs>`_
plugin for detailed information about behavior and supported features.

`Pages, sections and table of contents`_
----------------------------------------

Table of contents is unconditionally generated for all compound documentation
pages and includes both ``@section`` blocks in the detailed documentation as
well as the reference sections. If your documentation is using Markdown-style
headers (prefixed with ``##``, for example), the script is not able to generate
TOC entries for these. Upon encountering them, tt will warn and suggest to use
the ``@section`` command instead.

Table of contents for pages is generated only if they specify
``@tableofcontents`` in their documentation block.

`Namespace members in file scope`_
----------------------------------

Doxygen by default doesn't render namespace members for file documentation in
its XML output. To match the behavior of stock HTML output, enable the
:ini:`XML_NS_MEMB_FILE_SCOPE` Doxyfile option:

.. code:: ini

    XML_NS_MEMB_FILE_SCOPE = YES

.. note-warning:: Doxygen support

    In order to use the :ini:`XML_NS_MEMB_FILE_SCOPE` option, you need at least
    Doxygen 1.8.15.

Please note that APIs listed in file documentation pages lose all namespace
information (and when it appears in search, it's prefixed with the file name
instead of the namespace). This is due to several bugs on Doxygen side, which
still need to be patched (or worked around).

`Private virtual functions`_
----------------------------

Private virtual functions, if documented, are shown in the output as well, so
codebases can properly follow (`Virtuality guidelines by Herb Sutter <http://www.gotw.ca/publications/mill18.htm>`_)
To avoid also undocumented :cpp:`override`\ s showing in the output, you may
want to disable the :ini:`INHERIT_DOCS` Doxyfile option (which is enabled by
default). Also, please note that while privates are currently unconditionally
exported to the XML output, Doxygen doesn't allow linking to them by default
and you have to enable the :ini:`EXTRACT_PRIV_VIRTUAL` Doxyfile option:

.. code:: ini

    INHERIT_DOCS = NO
    EXTRACT_PRIV_VIRTUAL = YES

.. note-warning:: Doxygen support

    In order to use the :ini:`EXTRACT_PRIV_VIRTUAL` option, you need at least
    Doxygen 1.8.16.

`Inline namespaces`_
--------------------

:cpp:`inline namespace`\ s are marked as such in class/namespace index pages,
namespace documentation pages and nested namespace lists. Doxygen additionally
flattens those, so their contents appear in the parent namespace as well.

.. note-warning:: Doxygen support

    This feature requires Doxygen 1.8.19 or newer.

`Include files`_
----------------

Doxygen by default shows corresponding :cpp:`#include`\ s only for classes. The
m.css Doxygen theme shows it also for namespaces, free functions, enums,
typedefs, variables and :cpp:`#define`\ s. The rules are:

-   For classes, :cpp:`#include` information is always shown on the top
-   If a namespace doesn't contain any inner namespaces or classes and consists
    only of functions (enums, typedefs, variables) that are all declared in the
    same header file, the :cpp:`#include` information is shown only globally at
    the top, similarly to classes
-   If a namespace contains inner classes/namespaces, or is spread over multiple
    headers, the :cpp:`#include` information is shown locally for each member
-   Files don't show any include information, as it is known implicitly
-   In case of modules (grouped using ``@defgroup``), the :cpp:`#include` info
    is always shown locally for each member. This includes also :cpp:`#define`\ s.
-   In case of enums, typedefs, variables, functions and defines ``@related``
    to some class, these also have the :cpp:`#include` shown in case it's
    different from the class :cpp:`include`.

This feature is enabled by default, disable :ini:`SHOW_INCLUDE_FILES` in the
Doxyfile to hide all :cpp:`#include`-related information:

.. code:: ini

    SHOW_INCLUDE_FILES = NO

.. note-warning:: Doxygen support

    Doxygen 1.8.15 doesn't correctly provide location information for function
    and variable declarations and you might encounter issues. The behavior is
    fixed since 1.8.16.

`Code highlighting`_
--------------------

Every code snippet should be annotated with language-specific extension like in
the example below. If not, the theme will assume C++ and emit a warning on
output. Language of snippets included via ``@include`` and related commands is
autodetected from filename.

.. code:: c++

    /**
    @code{.cpp}
    int main() { }
    @endcode
    */

Besides native Pygments mapping of file extensions to languages, there are the
following special cases:

.. class:: m-table m-fullwidth

=================== ===========================================================
Filename suffix     Detected language
=================== ===========================================================
``.h``              C++ (instead of C)
``.h.cmake``        C++ (instead of CMake), as this extension is often used for
                    C++ headers that are preprocessed with CMake
``.glsl``           GLSL. For some reason, stock Pygments detect only
                    ``.vert``, ``.frag`` and ``.geo`` extensions as GLSL.
``.conf``           INI (key-value configuration files)
``.ansi``           `Colored terminal output <{filename}/css/components.rst#colored-terminal-output>`_.
                    Use ``.shell-session`` pseudo-extension for simple
                    uncolored terminal output.
``.xml-jinja``      Jinja templates in XML markup (these don't have any
                    well-known extension otherwise)
``.html-jinja``     Jinja templates in HTML markup (these don't have any
                    well-known extension otherwise)
``.jinja``          Jinja templates (these don't have any
                    well-known extension otherwise)
=================== ===========================================================

The theme has experimental support for inline code highlighting. Inline code is
distinguished from code blocks using the following rules:

-   Code that is delimited from surrounding paragraphs with an empty line is
    considered as block.
-   Code that is coming from ``@include``, ``@snippet`` and related commands
    that paste external file content is always considered as block.
-   Code that is coming from ``@code`` and is not alone in a paragraph is
    considered as inline.
-   For compatibility reasons, if code that is detected as inline consists of
    more than one line, it's rendered as code block and a warning is printed to
    output.

Inline highlighted code is written also using the ``@code`` command, but as
writing things like

.. code:: c++

    /** Returns @code{.cpp} Magnum::Vector2 @endcode, which is
        @code{.glsl} vec2 @endcode in GLSL. */

is too verbose, it's advised to configure some aliases in your ``Doxyfile-mcss``.
For example, you can configure an alias for general inline code snippets and
shorter versions for commonly used languages like C++ and CMake.

.. code:: ini

    ALIASES += \
        "cb{1}=@code{\1}" \
        "ce=@endcode" \
        "cpp=@code{.cpp}" \
        "cmake=@code{.cmake}"

With this in place the above could be then written simply as:

.. code:: c++

    /** Returns @cpp Magnum::Vector2 @ce, which is @cb{.glsl} vec2 @ce in GLSL. */

If you need to preserve compatibility with stock Doxygen HTML output (because
it renders all ``@code`` sections as blocks), use the following fallback
aliases in the original ``Doxyfile``:

.. code:: ini

    ALIASES += \
        "cb{1}=<tt>" \
        "ce=</tt>" \
        "cpp=<tt>" \
        "cmake=<tt>"

.. block-warning:: Doxygen limitations

    It's not possible to use inline code highlighting in ``@brief``
    description. Code placed there is moved by Doxygen to the detailed
    description. Similarly, it's not possible to use it in an ``@xrefitem``
    (``@todo``, ``@bug``...) paragraph --- code placed there is moved to a
    paragraph after (but it works as expected for ``@note`` and similar).

    It's not possible to put a ``@code`` block (delimited by blank lines) to a
    Markdown list. A workaround is to use explicit HTML markup instead. See
    `Content`_ for more information about list behavior.

    .. code-figure::

        .. code:: c++

            /**
            <ul>
            <li>
                A paragraph.

                @code{.cpp}
                #include <os>
                @endcode
            </li>
            <li>
                Another paragraph.

                Yet another
            </li>
            </ul>
            */

        .. raw:: html

            <ul>
              <li>
                <p>A paragraph.</p>
                <pre class="m-code"><span class="cp">#include</span> <span class="cpf">&lt;os&gt;</span><span class="cp"></span></pre>
              </li>
              <li>
                <p>Another paragraph.</p>
                <p>Yet another</p>
              </li>
            </ul>

`Theme-specific commands`_
--------------------------

It's possible to insert custom m.css classes into the Doxygen output. Add the
following to your ``Doxyfile-mcss``:

.. code:: ini

    ALIASES += \
        "m_div{1}=@xmlonly<mcss:div xmlns:mcss=\"http://mcss.mosra.cz/doxygen/\" mcss:class=\"\1\">@endxmlonly" \
        "m_enddiv=@xmlonly</mcss:div>@endxmlonly" \
        "m_span{1}=@xmlonly<mcss:span xmlns:mcss=\"http://mcss.mosra.cz/doxygen/\" mcss:class=\"\1\">@endxmlonly" \
        "m_endspan=@xmlonly</mcss:span>@endxmlonly" \
        "m_class{1}=@xmlonly<mcss:class xmlns:mcss=\"http://mcss.mosra.cz/doxygen/\" mcss:class=\"\1\" />@endxmlonly" \
        "m_footernavigation=@xmlonly<mcss:footernavigation xmlns:mcss=\"http://mcss.mosra.cz/doxygen/\" />@endxmlonly" \
        "m_examplenavigation{2}=@xmlonly<mcss:examplenavigation xmlns:mcss=\"http://mcss.mosra.cz/doxygen/\" mcss:page=\"\1\" mcss:prefix=\"\2\" />@endxmlonly" \
        "m_keywords{1}=@xmlonly<mcss:search xmlns:mcss=\"http://mcss.mosra.cz/doxygen/\" mcss:keywords=\"\1\" />@endxmlonly" \
        "m_keyword{3}=@xmlonly<mcss:search xmlns:mcss=\"http://mcss.mosra.cz/doxygen/\" mcss:keyword=\"\1\" mcss:title=\"\2\" mcss:suffix-length=\"\3\" />@endxmlonly" \
        "m_enum_values_as_keywords=@xmlonly<mcss:search xmlns:mcss=\"http://mcss.mosra.cz/doxygen/\" mcss:enum-values-as-keywords=\"true\" />@endxmlonly"

If you need backwards compatibility with stock Doxygen HTML output, just make
the aliases empty in your original ``Doxyfile``. Note that you can rename the
aliases however you want to fit your naming scheme.

.. code:: ini

    ALIASES += \
        "m_div{1}=" \
        "m_enddiv=" \
        "m_span{1}=" \
        "m_endspan=" \
        "m_class{1}=" \
        "m_footernavigation=" \
        "m_examplenavigation{2}" \
        "m_keywords{1}=" \
        "m_keyword{3}=" \
        "m_enum_values_as_keywords="

With ``@m_class`` it's possible to add CSS classes to the immediately following
paragraph, image, table, list or math formula block. When used before a block
such as ``@par``, ``@note``, ``@see`` or ``@xrefitem``, the CSS class fully
overrides the block styling. By default :css:`.m-note` with some color and
:html:`<h4>` is used, with ``@m_class`` before it get :html:`<h3>` for the
title and you can turn it into a block, for example:

.. code-figure::

    .. code:: c++

        /**
        @m_class{m-block m-success}

        @par Third-party license info
            This utility depends on the [Magnum engine](https://magnum.graphics).
            It's licensed under MIT, so all you need to do is mention it in the
            credits of your commercial app.
        */

    .. block-success:: Third-party license info

        This utility depends on the `Magnum engine <https://magnum.graphics>`_.
        It's licensed under MIT, so all you need to do is mention it in the
        credits of your commercial app.

When used inline, it affects the immediately following emphasis, strong text,
link or inline math formula. Example usage:

.. code-figure::

    .. code:: c++

        /**
        A green formula:

        @m_class{m-success}

        @f[
            e^{i \pi} + 1 = 0
        @f]

        Use the @m_class{m-label m-warning} **Shift** key.
        */

    .. role:: label-warning
        :class: m-label m-warning

    A green formula:

    .. math::
        :class: m-success

        e^{i \pi} + 1 = 0

    Use the :label-warning:`Shift` key.

.. note-info::

    Due to parsing ambiguities, in order to affect the whole block with
    ``@m_class`` instead of just the immediately following inline element, you
    have to separate it from the paragraph with a blank line:

    .. code:: c++

        /**
        @m_class{m-text m-green}

        @m_class{m-text m-big}
        **This text is big,** but the whole paragraph is green.
        */

The builtin ``@parblock`` command can be combined with ``@m_class`` to wrap a
block of HTML code in a :html:`<div>` and add CSS classes to it. With
``@m_div`` and ``@m_span`` it's possible to wrap individual paragraphs or
inline text in :html:`<div>` / :html:`<span>` and add CSS classes to them
without any extra elements being added. Example usage and corresponding
rendered HTML output:

.. code-figure::

    .. code:: c++

        /**
        @m_class{m-note m-dim m-text-center} @parblock
        This block is rendered in a dim note.

        Centered.
        @endparblock

        @m_div{m-button m-primary} <a href="https://doc.magnum.graphics/">@m_div{m-big}See
        it live! @m_enddiv @m_div{m-small} uses the m.css theme @m_enddiv </a> @m_enddiv

        This text contains a @span{m-text m-success} green @endspan word.
        */

    .. note-dim::
        :class: m-text-center

        This paragraph is rendered in a dim note.

        Centered.

    .. button-primary:: https://doc.magnum.graphics

        See it live!

        uses the m.css theme

    .. role:: success
        :class: m-text m-success

    This text contains a :success:`green` word.

.. note-warning::

    Note that due to Doxygen XML output limitations it's not possible to wrap
    multiple paragraphs with ``@m_div`` / ``@m_span``, attempt to do that will
    result in an invalid XML that can't be processed. Use the ``@parblock`` in
    that case instead. Similarly, if you forget a closing ``@m_enddiv`` /
    ``@m_endspan`` or misplace them, the result will be an invalid XML file.

It's possible to combine ``@par`` with ``@parblock`` to create blocks, notes
and other `m.css components <{filename}/css/components.rst>`_ with arbitrary
contents. The ``@par`` command visuals can be fully overridden by putting ``@m_class`` in front, the ``@parblock`` after will ensure everything will
belong inside. A bit recursive example:

.. code-figure::

    .. code:: c++

        /**
        @m_class{m-block m-success} @par How to get the answer
        @parblock
            It's simple:

            @m_class{m-code-figure} @parblock
                @code{.cpp}
                // this is the code
                printf("The answer to the universe and everything is %d.", 5*9);
                @endcode

                @code{.shell-session}
                The answer to the universe and everything is 42.
                @endcode
            @endparblock
        @endparblock
        */

    .. block-success:: How to get an answer

        It's simple:

        .. code-figure::

            .. code:: c++

                // this is the code
                printf("The answer to the universe and everything is %d.", 5*9);

            .. code:: shell-session

                The answer to the universe and everything is 42.

The ``@m_footernavigation`` command is similar to ``@tableofcontents``, but
across pages --- if a page is a subpage of some other page and this command is
present in page detailed description, it will cause the footer of the rendered
page to contain a link to previous, parent and next page according to defined
page order.

The ``@m_examplenavigation`` command is able to put breadcrumb navigation to
parent page(s) of ``@example`` listings in order to make it easier for users to
return back from example source code to a tutorial page, for example. When used
in combination with ``@m_footernavigation``, navigation to parent page and to
prev/next file of the same example is put at the bottom of the page. The
``@m_examplenavigation`` command takes two arguments, first is the parent page
for this example (used to build the breadcrumb and footer navigation), second
is example path prefix (which is then stripped from page title and is also used
to discover which example files belong together). Example usage --- the
``@m_examplenavigation`` and ``@m_footernavigation`` commands are simply
appended the an existing ``@example`` command.

.. code:: c++

    /**
    @example helloworld/CMakeLists.txt @m_examplenavigation{example,helloworld/} @m_footernavigation
    @example helloworld/configure.h.cmake @m_examplenavigation{example,helloworld/} @m_footernavigation
    @example helloworld/main.cpp @m_examplenavigation{example,helloworld/} @m_footernavigation
    */

The purpose of ``@m_keywords``, ``@m_keyword`` and ``@m_enum_values_as_keywords``
command is to add further search keywords to given documented symbols. Use
``@m_keywords`` to enter whitespace-separated list of keywords. The
``@m_enum_values_as_keywords`` command will add initializers of given enum
values as keywords for each corresponding value, it's ignored when not used in
enum description block. In the following example, an OpenGL wrapper API adds GL
API names as keywords for easier discoverability, so e.g. the
:cpp:`Texture2D::setStorage()` function is also found when typing
``glTexStorage2D()`` into the search field, or the :cpp:`Renderer::Feature::DepthTest`
enum value is found when entering :cpp:`GL_DEPTH_TEST`:

.. code:: c++

    /**
     * @brief Set texture storage
     *
     * @m_keywords{glTexStorage2D() glTextureStorage2D()}
     */
    Texture2D& Texture2D::setStorage(...);

    /**
     * @brief Renderer feature
     *
     * @m_enum_values_as_keywords
     */
    enum class RendererFeature: GLenum {
        /** Depth test */
        DepthTest = GL_DEPTH_TEST,

        ...
    };

The ``@m_keyword`` command is useful if you need to enter a keyword containing
spaces, the optional second and third parameter allow you to specify a
different title and suffix length. Example usage --- in the first case below,
the page will be discoverable both using its primary title and using
*TCB spline support*, in the second and third case the two overloads of the
:cpp:`lerp()` function are discoverable also via :cpp:`mix()`, displaying
either *GLSL mix()* or *GLSL mix(genType, genType, float)* in the search
results. The last parameter is suffix length, needed to correctly highlight the
*mix* substring when there are additional characters at the end of the title.
If not specified, it defaults to :cpp:`0`, meaning the search string is a
suffix of the title.

.. code:: c++

    /**
     * @page splines-tcb Kochanek–Bartels spline support
     * @m_keyword{TCB spline support,,}
     */

    /**
     * @brief Clamp a value
     * @m_keyword{clamp(),GLSL clamp(),}
     */
    float lerp(float x, float y, float a);

    /**
     * @brief Clamp a value
     * @m_keyword{mix(),GLSL mix(genType\, genType\, float),23}
     */
    template<class T> lerp(const T& x, const T& y, float a);

`Version labels`_
-----------------

By default, to keep compatibility with existing content, the ``@since`` block
is rendered as a note directly in the text flow. If you enable the
:ini:`M_SINCE_BADGES` option, content of the ``@since`` block is expected to be
a single label that can optionally link to a changelog entry. The label is then
placed visibly next to given entry and detailed description as well as all
listings. Additionally, if ``@since`` is followed by ``@deprecated``, it adds
version info to the deprecation notice (instead of denoting when given feature
was added) --- in this case it expects just a textual info, without any label
styling. Recommended use is in combination with an alias that takes care of the
label rendering and  For example (the ``@m_class`` is the same as described in
`Theme-specific commands`_ above):

.. code:: ini

    ALIASES = \
        "m_class{1}=@xmlonly<mcss:class xmlns:mcss=\"http://mcss.mosra.cz/doxygen/\" mcss:class=\"\1\" />@endxmlonly" \
        "m_since{2}=@since @m_class{m-label m-success m-flat} @ref changelog-\1-\2 \"since v\1.\2\"" \
        "m_deprecated_since{2}=@since deprecated in v\1.\2 @deprecated"

.. class:: m-noindent

in the Doxyfile, and the following in ``conf.py``:

.. code:: py

    VERSION_LABELS = True

With the above configuration, the following markup will render a
:label-flat-success:`since v1.3` label leading to a page named ``changelog-1-3``
next to both function entry and detailed docs in the first case, and a
:label-danger:`deprecated since v1.3` label in the second case:

.. code:: c++

    /**
    @brief Fast inverse square root
    @m_since{1,3}

    Faster than `1.0f/sqrt(a)`.
    */
    float fastinvsqrt(float a);

    /**
    @brief Slow inverse square root

    Attempts to figure out the value by a binary search.
    @m_deprecated_since{1,3} Use @ref fastinvsqrt() instead.
    */
    float slowinvsqrt(float a);

`Command-line options`_
=======================

.. code:: sh

    ./doxygen.py [-h] [--templates TEMPLATES] [--wildcard WILDCARD]
                 [--index-pages INDEX_PAGES [INDEX_PAGES ...]]
                 [--no-doxygen] [--search-no-subtree-merging]
                 [--search-no-lookahead-barriers]
                 [--search-no-prefix-merging] [--sort-globbed-files]
                 [--debug]
                 config

Arguments:

-   ``config`` --- where the Doxyfile or conf.py is

Options:

-   ``-h``, ``--help`` --- show this help message and exit
-   ``--templates TEMPLATES`` --- template directory. Defaults to the
    ``templates/doxygen/`` subdirectory if not set.
-   ``--wildcard WILDCARD`` --- only process files matching the wildcard.
    Useful for debugging to speed up / restrict the processing to a subset of
    files. Defaults to ``*.xml`` if not set.
-   ``--index-pages INDEX_PAGES [INDEX_PAGES ...]`` --- index page templates.
    By default, if not set, the index pages are matching stock Doxygen, i.e.
    ``annotated.html``, ``files.html``, ``modules.html``, ``namespaces.html``
    and ``pages.html``.
    See `Navigation page templates`_ section below for more information.
-   ``--no-doxygen`` --- don't run Doxygen before. By default Doxygen is run
    before the script to refresh the generated XML output.
-   ``--search-no-subtree-merging`` --- don't optimize search data size by
    merging subtrees
-   ``--search-no-lookahead-barriers`` --- don't insert search lookahead
    barriers that improve search result relevance
-   ``--search-no-prefix-merging`` --- don't merge search result prefixes
-   ``--sort-globbed-files`` --- sort globbed files for better reproducibility
-   ``--debug`` --- verbose logging output. Useful for debugging.

`Troubleshooting`_
==================

`Generated output is (mostly) empty`_
-------------------------------------

As stated in the `Features <#important-differences-to-stock-html-output>`_
section above; files, directories and symbols with no documentation are not
present in the output at all. In particular, when all your sources are under a
subdirectory and/or a namespace and that subdirectory / namespace is not
documented, the file / symbol tree will not show anything.

A simple ``@brief`` entry is enough to fix this. For example, if you have a
:cpp:`MorningCoffee::CleanCup` class that's available from
:cpp:`#include <MorningCoffee/CleanCup.h>`, these documentation blocks are
enough to have the directory, file, namespace and also the class appear in the
file / symbol tree:

.. code:: c++

    /** @dir MorningCoffee
     * @brief Namespace @ref MorningCoffee
     */
    /** @namespace MorningCoffee
     * @brief The Morning Coffee library
     */

.. code:: c++

    // CleanCup.h

    /** @file
     * @brief Class @ref CleanCup
     */

    namespace MorningCoffee {

        /**
         * @brief A clean cup
         */
        class CleanCup {

        ...

To help you debugging this, run ``doxygen.py`` with the ``--debug`` option.
and look for entries that look like below. Because there are many false
positives, this information is not present in the non-verbose output.

.. code:: shell-session

    DEBUG:root:dir_22305cb0964bbe63c21991dd2265ce48.xml: neither brief nor
    detailed description present, skipping

Alternatively, there's a possibility to just show everything, however that may
include also things you don't want to have shown. See
`Showing undocumented symbols and files`_ for more information.

`Output is not styled`_
-----------------------

If your ``Doxyfile`` contains a non-empty :ini:`HTML_EXTRA_STYLESHEET` option,
m.css will use CSS files from there instead of the builtin ones. Either
override it to an empty value in your ``Doxyfile-mcss`` or specify proper CSS
files explicitly as mentioned in the `Theme selection`_ section.

`Generating takes too long, crashes, asserts or generally fails`_
-----------------------------------------------------------------

The XML output generated by Doxygen is underspecified and unnecessarily
complex, so it might very well happen that your documentation triggers some
untested code path. The script is designed to fail early and hard instead of
silently continuing and producing wrong output --- if you see an assertion
failure or a crash or things seem to be stuck, you can do the following:

-   Re-run the script with the ``--debug`` option. That will list what XML file
    is being processed at the moment and helps you narrow down the issue to a
    particular file.
-   At the moment, math formula rendering is not batched and takes very long,
    as LaTeX is started separately for every occurrence.
    :gh:`Help in this area is welcome. <mosra/m.css#32>`
-   Try with a freshly generated ``Doxyfile``. If it stops happening, the
    problem might be related to some configuration option (but maybe also an
    alias or preprocessor :cpp:`#define` that's not defined anymore)
-   m.css currently expects only C++ input. If you have Python or some other
    language on input, it will get very confused very fast. This can be also
    caused by a file being included by accident, restrict the :ini:`INPUT` and
    :ini:`FILE_PATTERNS` options to prevent that.
-   Try to narrow the problem down to a small code snippet and
    `submit a bug report <https://github.com/mosra/m.css/issues/new>`_ with
    given snippet and all relevant info (especially Doxygen version). Or ask
    in the `Gitter chat <https://gitter.im/mosra/m.css>`_. If I'm not able to
    provide a fix right away, there's a great chance I've already seen such
    problem and can suggest a workaround at least.

`Customizing the template`_
===========================

The rest of the documentation explains how to customize the builtin template to
better suit your needs. Each documentation file is generated from one of the
template files that are bundled with the script. However, it's possible to
provide your own Jinja2 template files for customized experience as well as
modify the CSS styling.

`CSS files`_
------------

By default, compiled CSS files are used to reduce amount of HTTP requests and
bandwidth needed for viewing the documentation. However, for easier
customization and debugging it's better to use the unprocessed stylesheets. The
:ini:`HTML_EXTRA_STYLESHEET` lists all files that go to the :html:`<link rel="stylesheet" />`
in the resulting HTML markup, while :ini:`HTML_EXTRA_FILES` lists the
indirectly referenced files that need to be copied to the output as well. Below
is an example configuration corresponding to the dark theme:

.. code:: ini

    STYLESHEETS = [
        'https://fonts.googleapis.com/css?family=Source+Sans+Pro:400,400i,600,600i%7CSource+Code+Pro:400,400i,600',
        '../css/m-dark.css',
        '../css/m-documentation.css'
    ]
    EXTRA_FILES = [
        '../css/m-theme-dark.css',
        '../css/m-grid.css',
        '../css/m-components.css',
        '../css/m-layout.css',
        '../css/pygments-dark.css',
        '../css/pygments-console.css'
    ]
    THEME_COLOR = '#22272e'

After making desired changes to the source files, it's possible to postprocess
them back to the compiled version using the ``postprocess.py`` utility as
explained in the `CSS themes <{filename}/css/themes.rst#make-your-own>`_
documentation. In case of the dark theme, the ``m-dark+documentation.compiled.css``
and ``m-dark.documentation.compiled.css`` files are produced like this:

.. code:: sh

    cd css
    ./postprocess.py m-dark.css m-documentation.css -o m-dark+documentation.compiled.css
    ./postprocess.py m-dark.css m-documentation.css --no-import -o m-dark.documentation.compiled.css

`Compound documentation template`_
----------------------------------

For compound documentation one output HTML file corresponds to one input XML
file and there are some naming conventions imposed by Doxygen.

.. class:: m-table m-fullwidth

======================= =======================================================
Filename                Use
======================= =======================================================
``class.html``          Class documentation, read from ``class*.xml`` and saved
                        as ``class*.html``
``dir.html``            Directory documentation, read from ``dir_*.xml`` and
                        saved as ``dir_*.html``
``example.html``        Example code listing, read from ``*-example.xml`` and
                        saved as ``*-example.html``
``file.html``           File documentation, read from ``*.xml`` and saved as
                        ``*.html``
``namespace.html``      Namespace documentation, read from ``namespace*.xml``
                        and saved as ``namespace*.html``
``group.html``          Module documentation, read from ``group_*.xml``
                        and saved as ``group_*.html``
``page.html``           Page, read from ``*.xml``/``indexpage.xml`` and saved
                        as ``*.html``/``index.html``
``struct.html``         Struct documentation, read from ``struct*.xml`` and
                        saved as ``struct*.html``
``union.html``          Union documentation, read from ``union*.xml`` and saved
                        as ``union*.html``
======================= =======================================================

Each template is passed a subset of the ``Doxyfile`` and ``conf.py``
configuration values from the `Configuration`_ tables. Most values are provided
as-is depending on their type, so either strings, booleans, or lists of
strings. The exceptions are:

-   The :py:`LINKS_NAVBAR1` and :py:`LINKS_NAVBAR2` are processed to tuples in
    a form :py:`(html, title, url, id, sub)` where either :py:`html` is a full
    HTML code for the link and :py:`title`, :py:`url` :py:`id` is empty; or
    :py:`html` is :py:`None`, :py:`title` and :py:`url` is a link title and URL
    and :py:`id` is compound ID (to use for highlighting active menu item). The
    last item, :py:`sub` is a list optionally containing sub-menu items. The
    sub-menu items are in a similarly formed tuple,
    :py:`(html, title, url, id)`.
-   The :py:`FAVICON` is converted to a tuple of :py:`(url, type)` where
    :py:`url` is the favicon URL and :py:`type` is favicon MIME type to
    populate the ``type`` attribute of :html:`<link rel="favicon" />`.

.. class:: m-noindent

and in addition the following variables:

.. class:: m-table m-fullwidth

=========================== ===================================================
Variable                    Description
=========================== ===================================================
:py:`FILENAME`              Name of given output file
:py:`DOXYGEN_VERSION`       Version of Doxygen that generated given XML file
=========================== ===================================================

In addition to builtin Jinja2 filters, the ``basename_or_url`` filter returns
either a basename of file path, if the path is relative; or a full URL, if the
argument is an absolute URL. It's useful in cases like this:

.. code:: html+jinja

  {% for css in STYLESHEETS %}
  <link rel="stylesheet" href="{{ css|basename_or_url }}" />
  {% endfor %}

The actual page contents are provided in a :py:`compound` object, which has the
following properties. All exposed data are meant to be pasted directly to the
HTML code without any escaping.

.. class:: m-table m-fullwidth

======================================= =======================================
Property                                Description
======================================= =======================================
:py:`compound.kind`                     One of :py:`'class'`, :py:`'dir'`,
                                        :py:`'example'`, :py:`'file'`,
                                        :py:`'group'`, :py:`'namespace'`,
                                        :py:`'page'`, :py:`'struct'`,
                                        :py:`'union'`, used to choose a
                                        template file from above
:py:`compound.id`                       Unique compound identifier, usually
                                        corresponding to output file name
:py:`compound.url`                      Compound URL (or where this file will
                                        be saved)
:py:`compound.include`                  Corresponding :cpp:`#include` statement
                                        to use given compound. Set only for
                                        classes or namespaces that are all
                                        defined in a single file. See
                                        `Include properties`_ for details.
:py:`compound.name`                     Compound name
:py:`compound.templates`                Template specification. Set only for
                                        classes. See `Template properties`_ for
                                        details.
:py:`compound.has_template_details`     If there is a detailed documentation
                                        of template parameters
:py:`compound.sections`                 Sections of detailed description. See
                                        `Navigation properties`_ for details.
:py:`compound.footer_navigation`        Footer navigation of a page. See
                                        `Navigation properties`_ for details.
:py:`compound.brief`                    Brief description. Can be empty. [1]_
:py:`compound.is_inline`                Whether the namespace is :cpp:`inline`.
                                        Set only for namespaces.
:py:`compound.is_final`                 Whether the class is :cpp:`final`. Set
                                        only for classes.
:py:`compound.deprecated`               Deprecation status [7]_
:py:`compound.since`                    Since which version the compound is
                                        available [8]_
:py:`compound.description`              Detailed description. Can be empty. [2]_
:py:`compound.modules`                  List of submodules in this compound.
                                        Set only for modules. See
                                        `Module properties`_ for details.
:py:`compound.dirs`                     List of directories in this compound.
                                        Set only for directories. See
                                        `Directory properties`_ for details.
:py:`compound.files`                    List of files in this compound. Set
                                        only for directories and files. See
                                        `File properties`_ for details.
:py:`compound.namespaces`               List of namespaces in this compound.
                                        Set only for files and namespaces. See
                                        `Namespace properties`_ for details.
:py:`compound.classes`                  List of classes in this compound. Set
                                        only for files and namespaces. See
                                        `Class properties`_ for details.
:py:`compound.base_classes`             List of base classes in this compound.
                                        Set only for classes. See
                                        `Class properties`_ for details.
:py:`compound.derived_classes`          List of derived classes in this
                                        compound. Set only for classes. See
                                        `Class properties`_ for details.
:py:`compound.enums`                    List of enums in this compound. Set
                                        only for files and namespaces. See
                                        `Enum properties`_ for details.
:py:`compound.typedefs`                 List of typedefs in this compound. Set
                                        only for files and namespaces. See
                                        `Typedef properties`_ for details.
:py:`compound.funcs`                    List of functions in this compound. Set
                                        only for files and namespaces. See
                                        `Function properties`_ for details.
:py:`compound.vars`                     List of variables in this compound. Set
                                        only for files and namespaces. See
                                        `Variable properties`_ for details.
:py:`compound.defines`                  List of defines in this compound. Set
                                        only for files. See `Define properties`_
                                        for details.
:py:`compound.public_types`             List of public types. Set only for
                                        classes. See `Type properties`_ for
                                        details.
:py:`compound.public_static_funcs`      List of public static functions. Set
                                        only for classes. See
                                        `Function properties`_ for details.
:py:`compound.public_funcs`             List of public functions. Set only for
                                        classes. See `Function properties`_ for
                                        details.
:py:`compound.signals`                  List of Qt signals. Set only for
                                        classes. See `Function properties`_ for
                                        details.
:py:`compound.public_slots`             List of public Qt slots. Set only for
                                        classes. See `Function properties`_ for
                                        details.
:py:`compound.public_static_vars`       List of public static variables. Set
                                        only for classes. See
                                        `Variable properties`_ for details.
:py:`compound.public_vars`              List of public variables. Set only for
                                        classes. See `Variable properties`_ for
                                        details.
:py:`compound.protected_types`          List of protected types. Set only for
                                        classes. See `Type properties`_ for
                                        details.
:py:`compound.protected_static_funcs`   List of protected static functions. Set
                                        only for classes. See
                                        `Function properties`_ for details.
:py:`compound.protected_funcs`          List of protected functions. Set only
                                        for classes. See `Function properties`_
                                        for details.
:py:`compound.protected_slots`          List of protected Qt slots. Set only
                                        for classes. See `Function properties`_
                                        for details.
:py:`compound.protected_static_vars`    List of protected static variables. Set
                                        only for classes. See
                                        `Variable properties`_ for details.
:py:`compound.protected_vars`           List of protected variables. Set only
                                        for classes. See `Variable properties`_
                                        for details.
:py:`compound.private_funcs`            List of documented private virtual
                                        functions. Set only for classes. See
                                        `Function properties`_ for details.
:py:`compound.private_slots`            List of documented private virtual Qt
                                        slots. Set only for classes. See
                                        `Function properties`_ for details.
:py:`compound.friend_funcs`             List of documented friend functions.
                                        Set only for classes. See
                                        `Function properties`_ for details.
:py:`compound.related`                  List of related non-member symbols. Set
                                        only for classes. See
                                        `Related properties`_ for details.
:py:`compound.groups`                   List of user-defined groups in this
                                        compound. See `Group properties`_ for
                                        details.
:py:`compound.has_enum_details`         If there is at least one enum with full
                                        description block [5]_
:py:`compound.has_typedef_details`      If there is at least one typedef with
                                        full description block [5]_
:py:`compound.has_func_details`         If there is at least one function with
                                        full description block [5]_
:py:`compound.has_var_details`          If there is at least one variable with
                                        full description block [5]_
:py:`compound.has_define_details`       If there is at least one define with
                                        full description block [5]_
:py:`compound.breadcrumb`               List of :py:`(title, URL)` tuples for
                                        breadcrumb navigation. Set only for
                                        classes, directories, files, namespaces
                                        and pages.
:py:`compound.prefix_wbr`               Fully-qualified symbol prefix for given
                                        compound with trailing ``::`` with
                                        :html:`<wbr/>` tag before every ``::``.
                                        Set only for classes, namespaces,
                                        structs and unions; on templated
                                        classes contains also the list of
                                        template parameter names.
======================================= =======================================

`Navigation properties`_
````````````````````````

The :py:`compound.sections` property defines a Table of Contents for given
detailed description. It's a list of :py:`(id, title, children)` tuples, where
:py:`id` is the link anchor, :py:`title` is section title and :py:`children` is
a recursive list of nested sections. If the list is empty, given detailed
description either has no sections or the TOC was not explicitly requested via
``@tableofcontents`` in case of pages.

The :py:`compound.footer_navigation` property defines footer navigation
requested by the ``@m_footernavigation`` `theme-specific command <#theme-specific-commands>`_.
If available, it's a tuple of :py:`(prev, up, next)` where each item is a tuple
of :py:`(url, title)` for a page that's either previous in the defined order,
one level up or next. For starting/ending page the :py:`prev`/:py:`next` is
:py:`None`.

`Include properties`_
`````````````````````

The :py:`compound.include` property is a tuple of :py:`(name, URL)` where
:py:`name` is the include name (together with angle brackets, quotes or a macro
name) and :py:`URL` is a URL pointing to the corresponding header documentation
file. This property is present only if the corresponding header documentation
is present. This property is present for classes; namespaces have it only when
all documented namespace contents are defined in a single file. For modules and
namespaces spread over multiple files this property is presented separately for
each enum, typedef, function, variable or define inside given module or
namespace. Directories, files and file members don't provide this property,
since in that case the mapping to a corresponding :cpp:`#include` file is known
implicitly.

`Module properties`_
````````````````````

The :py:`compound.modules` property contains a list of modules, where every
item has the following properties:

.. class:: m-table m-fullwidth

=========================== ===================================================
Property                    Description
=========================== ===================================================
:py:`module.url`            URL of the file containing detailed module docs
:py:`module.name`           Module name (just the leaf)
:py:`module.brief`          Brief description. Can be empty. [1]_
:py:`module.deprecated`     Deprecation status [7]_
:py:`module.since`          Since which version the module is available [8]_
=========================== ===================================================

`Directory properties`_
```````````````````````

The :py:`compound.dirs` property contains a list of directories, where every
item has the following properties:

.. class:: m-table m-fullwidth

=========================== ===================================================
Property                    Description
=========================== ===================================================
:py:`dir.url`               URL of the file containing detailed directory docs
:py:`dir.name`              Directory name (just the leaf)
:py:`dir.brief`             Brief description. Can be empty. [1]_
:py:`dir.deprecated`        Deprecation status [7]_
:py:`dir.since`             Since which version the directory is available [8]_
=========================== ===================================================

`File properties`_
``````````````````

The :py:`compound.files` property contains a list of files, where every item
has the following properties:

.. class:: m-table m-fullwidth

=========================== ===================================================
Property                    Description
=========================== ===================================================
:py:`file.url`              URL of the file containing detailed file docs
:py:`file.name`             File name (just the leaf)
:py:`file.brief`            Brief description. Can be empty. [1]_
:py:`file.deprecated`       Deprecation status [7]_
:py:`file.since`            Since which version the file is available [8]_
=========================== ===================================================

`Namespace properties`_
```````````````````````

The :py:`compound.namespaces` property contains a list of namespaces, where
every item has the following properties:

.. class:: m-table m-fullwidth

=============================== ===============================================
Property                        Description
=============================== ===============================================
:py:`namespace.url`             URL of the file containing detailed namespace
                                docs
:py:`namespace.name`            Namespace name. Fully qualified in case it's in
                                a file documentation, just the leaf name if in
                                a namespace documentation.
:py:`namespace.brief`           Brief description. Can be empty. [1]_
:py:`namespace.deprecated`      Deprecation status [7]_
:py:`namespace.since`           Since which version the namespace is available
                                [8]_
:py:`namespace.is_inline`       Whether this is an :cpp:`inline` namespace
=============================== ===============================================

`Class properties`_
```````````````````

The :py:`compound.classes` property contains a list of classes, where every
item has the following properties:

.. class:: m-table m-fullwidth

=========================== ===================================================
Property                    Description
=========================== ===================================================
:py:`class.kind`            One of :py:`'class'`, :py:`'struct'`, :py:`'union'`
:py:`class.url`             URL of the file containing detailed class docs
:py:`class.name`            Class name. Fully qualified in case it's in a file
                            documentation, just the leaf name if in a namespace
                            documentation.
:py:`class.templates`       Template specification. See `Template properties`_
                            for details.
:py:`class.brief`           Brief description. Can be empty. [1]_
:py:`class.deprecated`      Deprecation status [7]_
:py:`class.since`           Since which version the class is available [8]_
:py:`class.is_protected`    Whether this is a protected base class. Set only
                            for base classes.
:py:`class.is_virtual`      Whether this is a virtual base class or a
                            virtually derived class. Set only for base /
                            derived classes.
:py:`class.is_final`        Whether this is a final derived class. Set only for
                            derived classes.
=========================== ===================================================

`Enum properties`_
``````````````````

The :py:`compound.enums` property contains a list of enums, where every item
has the following properties:

.. class:: m-table m-fullwidth

=============================== ===============================================
Property                        Description
=============================== ===============================================
:py:`enum.base_url`             Base URL of file containing detailed
                                description [3]_
:py:`enum.include`              Corresponding :cpp:`#include` to get the enum
                                definition. Present only for enums inside
                                modules or inside namespaces that are spread
                                over multiple files.
                                See `Include properties`_ for more information.
:py:`enum.id`                   Identifier hash [3]_
:py:`enum.type`                 Enum type or empty if implicitly typed [6]_
:py:`enum.is_strong`            If the enum is strong
:py:`enum.name`                 Enum name [4]_
:py:`enum.brief`                Brief description. Can be empty. [1]_
:py:`enum.description`          Detailed description. Can be empty. [2]_
:py:`enum.has_details`          If there is enough content for the full
                                description block [5]_
:py:`enum.deprecated`           Deprecation status [7]_
:py:`enum.since`                Since which version the enum is available [8]_
:py:`enum.is_protected`         If the enum is :cpp:`protected`. Set only for
                                member types.
:py:`enum.values`               List of enum values
:py:`enum.has_value_details`    If the enum values have description
=============================== ===============================================

Every item of :py:`enum.values` has the following properties:

.. class:: m-table m-fullwidth

=========================== ===================================================
Property                    Description
=========================== ===================================================
:py:`value.base_url`        Base URL of file containing detailed description
                            [3]_
:py:`value.id`              Identifier hash [3]_
:py:`value.name`            Value name [4]_
:py:`value.initializer`     Value initializer. Can be empty. [1]_
:py:`value.deprecated`      Deprecation status [7]_
:py:`value.since`           Since which version the value is available [8]_
:py:`value.brief`           Brief description. Can be empty. [1]_
:py:`value.description`     Detailed description. Can be empty. [2]_
=========================== ===================================================

`Typedef properties`_
`````````````````````

The :py:`compound.typedefs` property contains a list of typedefs, where every
item has the following properties:

.. class:: m-table m-fullwidth

=================================== ===========================================
Property                            Description
=================================== ===========================================
:py:`typedef.base_url`              Base URL of file containing detailed
                                    description [3]_
:py:`typedef.include`               Corresponding :cpp:`#include` to get the
                                    typedef declaration. Present only for
                                    typedefs inside modules or inside
                                    namespaces that are spread over multiple
                                    files. See `Include properties`_ for more
                                    information.
:py:`typedef.id`                    Identifier hash [3]_
:py:`typedef.is_using`              Whether it is a :cpp:`typedef` or an
                                    :cpp:`using`
:py:`typedef.type`                  Typedef type, or what all goes before the
                                    name for function pointer typedefs [6]_
:py:`typedef.args`                  Typedef arguments, or what all goes after
                                    the name for function pointer typedefs [6]_
:py:`typedef.name`                  Typedef name [4]_
:py:`typedef.templates`             Template specification. Set only in case of
                                    :cpp:`using`. . See `Template properties`_
                                    for details.
:py:`typedef.has_template_details`  If template parameters have description
:py:`typedef.brief`                 Brief description. Can be empty. [1]_
:py:`typedef.deprecated`            Deprecation status [7]_
:py:`typedef.since`                 Since which version the typedef is
                                    available [8]_
:py:`typedef.description`           Detailed description. Can be empty. [2]_
:py:`typedef.has_details`           If there is enough content for the full
                                    description block [4]_
:py:`typedef.is_protected`          If the typedef is :cpp:`protected`. Set
                                    only for member types.
=================================== ===========================================

`Function properties`_
``````````````````````

The :py:`commpound.funcs`, :py:`compound.public_static_funcs`,
:py:`compound.public_funcs`, :py:`compound.signals`,
:py:`compound.public_slots`, :py:`compound.protected_static_funcs`,
:py:`compound.protected_funcs`, :py:`compound.protected_slots`,
:py:`compound.private_funcs`, :py:`compound.private_slots`,
:py:`compound.friend_funcs` and :py:`compound.related_funcs` properties contain
a list of functions, where every item has the following properties:

.. class:: m-table m-fullwidth

=================================== ===========================================
Property                            Description
=================================== ===========================================
:py:`func.base_url`                 Base URL of file containing detailed
                                    description [3]_
:py:`func.include`                  Corresponding :cpp:`#include` to get the
                                    function declaration. Present only for
                                    functions inside modules or inside
                                    namespaces that are spread over multiple
                                    files. See `Include properties`_ for more
                                    information.
:py:`func.id`                       Identifier hash [3]_
:py:`func.type`                     Function return type [6]_
:py:`func.name`                     Function name [4]_
:py:`func.templates`                Template specification. See
                                    `Template properties`_ for details.
:py:`func.has_template_details`     If template parameters have description
:py:`func.params`                   List of function parameters. See below for
                                    details.
:py:`func.has_param_details`        If function parameters have description
:py:`func.return_value`             Return value description. Can be empty.
:py:`func.return_values`            Description of particular return values.
                                    See below for details.
:py:`func.exceptions`               Description of particular exception types.
                                    See below for details.
:py:`func.brief`                    Brief description. Can be empty. [1]_
:py:`func.description`              Detailed description. Can be empty. [2]_
:py:`func.has_details`              If there is enough content for the full
                                    description block [5]_
:py:`func.prefix`                   Function signature prefix, containing
                                    keywords such as :cpp:`static`. Information
                                    about :cpp:`constexpr`\ ness,
                                    :cpp:`explicit`\ ness and
                                    :cpp:`virtual`\ ity is removed from the
                                    prefix and available via other properties.
:py:`func.suffix`                   Function signature suffix, containing
                                    keywords such as :cpp:`const` and r-value
                                    overloads. Information about
                                    :cpp:`noexcept`, pure :cpp:`virtual`\ ity
                                    and :cpp:`delete`\ d / :cpp:`default`\ ed
                                    functions is removed from the suffix and
                                    available via other properties.
:py:`func.deprecated`               Deprecation status [7]_
:py:`func.since`                    Since which version the function is
                                    available [8]_
:py:`func.is_protected`             If the function is :cpp:`protected`. Set
                                    only for member functions.
:py:`func.is_private`               If the function is :cpp:`private`. Set only
                                    for member functions.
:py:`func.is_explicit`              If the function is :cpp:`explicit`. Set
                                    only for member functions.
:py:`func.is_virtual`               If the function is :cpp:`virtual` (or pure
                                    virtual). Set only for member functions.
:py:`func.is_pure_virtual`          If the function is pure :cpp:`virtual`. Set
                                    only for member functions.
:py:`func.is_override`              If the function is an :cpp:`override`. Set
                                    only for member functions.
:py:`func.is_final`                 If the function is a :cpp:`final override`.
                                    Set only for member functions.
:py:`func.is_noexcept`              If the function is :cpp:`noexcept` (even
                                    conditionally)
:py:`func.is_conditional_noexcept`  If the function is conditionally
                                    :cpp:`noexcept`.
:py:`func.is_constexpr`             If the function is :cpp:`constexpr`
:py:`func.is_defaulted`             If the function is :cpp:`default`\ ed
:py:`func.is_deleted`               If the function is :cpp:`delete`\ d
:py:`func.is_signal`                If the function is a Qt signal. Set only
                                    for member functions.
:py:`func.is_slot`                  If the function is a Qt slot. Set only for
                                    member functions.
=================================== ===========================================

The :py:`func.params` is a list of function parameters and their description.
Each item has the following properties:

.. class:: m-table m-fullwidth

=========================== ===================================================
Property                    Description
=========================== ===================================================
:py:`param.name`            Parameter name (if not anonymous)
:py:`param.type`            Parameter type, together with array specification
                            [6]_
:py:`param.type_name`       Parameter type, together with name and array
                            specification [6]_
:py:`param.default`         Default parameter value, if any [6]_
:py:`param.description`     Optional parameter description. If set,
                            :py:`func.has_param_details` is set as well.
:py:`param.direction`       Parameter direction. One of :py:`'in'`, :py:`'out'`,
                            :py:`'inout'` or :py:`''` if unspecified.
=========================== ===================================================

The :py:`func.return_values` property is a list of return values and their
description (in contract to :py:`func.return_value`, which is just a single
description). Each item is a tuple of :py:`(value, description)`. Can be empty,
it can also happen that both :py:`func.return_value` and :py:`func.return_values`
are resent. Similarly, the :py:`func.exceptions` property is a list of
:py:`(type, description)` tuples.

`Variable properties`_
``````````````````````

The :py:`compound.vars`, :py:`compound.public_vars` and
:py:`compound.protected_vars` properties contain a list of variables, where
every item has the following properties:

.. class:: m-table m-fullwidth

=============================== ===============================================
Property                        Description
=============================== ===============================================
:py:`var.base_url`              Base URL of file containing detailed
                                description [3]_
:py:`var.include`               Corresponding :cpp:`#include` to get the
                                variable declaration. Present only for
                                variables inside modules or inside namespaces
                                that are spread over multiple files. See
                                `Include properties`_ for more information.
:py:`var.id`                    Identifier hash [3]_
:py:`var.type`                  Variable type [6]_
:py:`var.name`                  Variable name [4]_
:py:`var.templates`             Template specification for C++14 variable
                                templates. See `Template properties`_ for
                                details.
:py:`var.has_template_details`  If template parameters have description
:py:`var.brief`                 Brief description. Can be empty. [1]_
:py:`var.description`           Detailed description. Can be empty. [2]_
:py:`var.has_details`           If there is enough content for the full
                                description block [5]_
:py:`var.deprecated`            Deprecation status [7]_
:py:`var.since`                 Since which version the variable is available
                                [8]_
:py:`var.is_static`             If the variable is :cpp:`static`. Set only for
                                member variables.
:py:`var.is_protected`          If the variable is :cpp:`protected`. Set only
                                for member variables.
:py:`var.is_constexpr`          If the variable is :cpp:`constexpr`
=============================== ===============================================

`Define properties`_
````````````````````

The :py:`compound.defines` property contains a list of defines, where every
item has the following properties:

.. class:: m-table m-fullwidth

=============================== ===============================================
Property                        Description
=============================== ===============================================
:py:`define.include`            Corresponding :cpp:`#include` to get the
                                define definition. Present only for defines
                                inside modules, since otherwise the define is
                                documented inside a file docs and the
                                corresponding include is known implicitly. See
                                `Include properties`_ for more information.
:py:`define.id`                 Identifier hash [3]_
:py:`define.name`               Define name
:py:`define.params`             List of macro parameter names. See below for
                                details.
:py:`define.has_param_details`  If define parameters have description
:py:`define.return_value`       Return value description. Can be empty.
:py:`define.brief`              Brief description. Can be empty. [1]_
:py:`define.description`        Detailed description. Can be empty. [2]_
:py:`define.deprecated`         Deprecation status [7]_
:py:`define.since`              Since which version the define is available
                                [8]_
:py:`define.has_details`        If there is enough content for the full
                                description block [5]_
=============================== ===============================================

The :py:`define.params` is set to :py:`None` if the macro is just a variable.
If it's a function, each item is a tuple consisting of name and optional
description. If the description is set, :py:`define.has_param_details` is set
as well. You can use :jinja:`{% if define.params != None %}` to disambiguate
between preprocessor macros and variables in your code.

`Type properties`_
``````````````````

For classes, the :py:`compound.public_types` and :py:`compound.protected_types`
contains a list of :py:`(kind, type)` tuples, where ``kind`` is one of
:py:`'class'`, :py:`'enum'` or :py:`'typedef'` and ``type`` is a corresponding
type of object described above.

`Template properties`_
``````````````````````

The :py:`compound.templates`, :py:`typedef.templates` and :py:`func.templates`
properties contain either :py:`None` if given symbol is a full template
specialization or a list of template parameters, where every item has the
following properties:

.. class:: m-table m-fullwidth

=========================== ===================================================
Property                    Description
=========================== ===================================================
:py:`template.type`         Template parameter type (:cpp:`class`,
                            :cpp:`typename` or a type)
:py:`template.name`         Template parameter name
:py:`template.default`      Template default value. Can be empty.
:py:`template.description`  Optional template description. If set,
                            :py:`i.has_template_details` is set as well.
=========================== ===================================================

You can use :jinja:`{% if i.templates != None %}` to test for the field
presence in your code.

`Related properties`_
`````````````````````

The :py:`compound.related` contains a list of related non-member symbols. Each
symbol is a tuple of :py:`(kind, member)`, where :py:`kind` is one of
:py:`'dir'`, :py:`'file'`, :py:`'namespace'`, :py:`'class'`, :py:`'enum'`,
:py:`'typedef'`, :py:`'func'`, :py:`'var'` or :py:`'define'` and :py:`member`
is a corresponding type of object described above.

`Group properties`_
```````````````````

The :py:`compound.groups` contains a list of user-defined groups. Each item has
the following properties:

.. class:: m-table m-fullwidth

======================= =======================================================
Property                Description
======================= =======================================================
:py:`group.id`          Group identifier [3]_
:py:`group.name`        Group name
:py:`group.description` Group description [2]_
:py:`group.members`     Group members. Each item is a tuple of
                        :py:`(kind, member)`, where :py:`kind` is one of
                        :py:`'namespace'`, :py:`'class'`, :py:`'enum'`,
                        :py:`'typedef'`, :py:`'func'`, :py:`'var'` or
                        :py:`'define'` and :py:`member` is a corresponding type
                        of object described above.
======================= =======================================================

`Navigation page templates`_
----------------------------

By default the theme tries to match the original Doxygen listing pages. These
pages are generated from the ``index.xml`` file and their template name
corresponds to output file name.

.. class:: m-table m-fullwidth

======================= =======================================================
Filename                Use
======================= =======================================================
``annotated.html``      Class listing
``files.html``          File and directory listing
``modules.html``        Module listing
``namespaces.html``     Namespace listing
``pages.html``          Page listing
======================= =======================================================

By default it's those five pages, but you can configure any other pages via the
``--index-pages`` option as mentioned in the `Command-line options`_ section.

Each template is passed a subset of the ``Doxyfile`` and ``conf.py``
configuration values from the `Configuration`_ tables and in addition the
:py:`FILENAME` and :py:`DOXYGEN_VERSION` variables as above. The navigation
tree is provided in an :py:`index` object, which has the following properties:

.. class:: m-table m-fullwidth

=========================== ===================================================
Property                    Description
=========================== ===================================================
:py:`index.symbols`         List of all namespaces + classes
:py:`index.files`           List of all dirs + files
:py:`index.pages`           List of all pages
:py:`index.modules`         List of all modules
=========================== ===================================================

The form of each list entry is the same:

.. class:: m-table m-fullwidth

=============================== ===============================================
Property                        Description
=============================== ===============================================
:py:`i.kind`                    Entry kind (one of :py:`'namespace'`,
                                :py:`'group'`, :py:`'class'`, :py:`'struct'`,
                                :py:`'union'`, :py:`'dir'`, :py:`'file'`,
                                :py:`'page'`)
:py:`i.name`                    Name
:py:`i.url`                     URL of the file with detailed documentation
:py:`i.brief`                   Brief documentation
:py:`i.deprecated`              Deprecation status [7]_
:py:`i.since`                   Since which version the entry is available [8]_
:py:`i.is_inline`               Whether this is an :cpp:`inline` namespace. Set
                                only for namespaces.
:py:`i.is_final`                Whether the class is :cpp:`final`. Set only for
                                classes.
:py:`i.has_nestable_children`   If the list has nestable children (i.e., dirs
                                or namespaces)
:py:`i.children`                Recursive list of child entries
=============================== ===============================================

Each list is ordered in a way that all namespaces are before all classes and
all directories are before all files.

---------------------------

.. [1] :py:`i.brief` is a single-line paragraph without the enclosing :html:`<p>`
    element, rendered as HTML. Can be empty in case of function overloads.
.. [2] :py:`i.description` is HTML code with the full description, containing
    paragraphs, notes, code blocks, images etc. Can be empty in case just the
    brief description is present.
.. [3] :py:`i.base_url`, joined using ``#`` with :py:`i.id` form a unique URL
    for given symbol. If the :py:`i.base_url` is not the same as
    :py:`compound.url`, it means given symbol is just referenced from given
    compound and its detailed documentation resides elsewhere.
.. [4] :py:`i.name` is just the member name, not qualified. Prepend
    :py:`compound.prefix_wbr` to it to get the fully qualified name.
.. [5] :py:`compound.has_*_details` and :py:`i.has_details` are :py:`True` if
    there is detailed description, function/template/macro parameter
    documentation, enum value listing or an entry-specific :cpp:`#include` that
    makes it worth to render the full description block. If :py:`False`, the
    member should be included only in the brief listing on top of the page to
    avoid unnecessary repetition. If :py:`i.base_url` is not the same as
    :py:`compound.url`, its :py:`i.has_details` is always set to :py:`False`.
.. [6] :py:`i.type` and :py:`param.default` is rendered as HTML and usually
    contains links to related documentation
.. [7] :py:`i.deprecated` is set to a string containing the deprecation
    status if detailed docs of given symbol contain the ``@deprecated`` command and to :py:`None` otherwise.  See also `Version labels`_ for more
    information.
.. [8] :py:`i.status` HTML code with a label or :py:`None`. See
    `Version labels`_ for more information.
