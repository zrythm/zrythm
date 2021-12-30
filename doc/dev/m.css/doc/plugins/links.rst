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

Links and other
###############

:breadcrumb: {filename}/plugins.rst Plugins
:footer:
    .. note-dim::
        :class: m-text-center

        `« Plots and graphs <{filename}/plugins/plots-and-graphs.rst>`_ | `Plugins <{filename}/plugins.rst>`_ | `Metadata » <{filename}/plugins/metadata.rst>`_

.. role:: html(code)
    :language: html
.. role:: py(code)
    :language: py
.. role:: rst(code)
    :language: rst
.. role:: ini(code)
    :language: ini

m.css plugins make linking to external content almost too easy. If your website
is about coding, chances are quite high that you will be linking to
repositories, documentation or bugtrackers. Manually copy-pasting links from
the browser gets quite annoying after a while and also doesn't really help with
keeping the reST sources readable.

Because not everybody needs to link to all services provided here, the
functionality is separated into a bunch of separate plugins, each having its
own requirements.

.. contents::
    :class: m-block m-default

.. note-warning::

    None of these plugins can be supported in the Doxygen theme (except the
    `Doxygen documentation`_ links, which are builtin). Instead, you can have
    some limited templating options by adding a custom command to the
    :ini:`ALIASES` Doxyfile option.

`Stylable links`_
=================

For Pelican, download the `m/link.py <{filename}/plugins.rst>`_ file, put it
including the ``m/`` directory into one of your :py:`PLUGIN_PATHS` and add
:py:`m.link` package to your :py:`PLUGINS` in ``pelicanconf.py``. For the
Python doc theme, it's enough to just list it in :py:`PLUGINS`:

.. code:: python

    PLUGINS += ['m.link']

The :rst:`:link:` interpreted text role is an alternative to the builtin
:abbr:`reST <reStructuredText>` link syntax, but with an ability to specify
additional CSS classes. At the moment the plugin knows only external URLs.

.. code-figure::

    .. code:: rst

        .. role:: link-flat-big(link)
            :class: m-flat m-text m-big

        :link-flat-big:`Look here, this is great! <https://mcss.mosra.cz>`

    .. role:: link-flat-big(link)
        :class: m-flat m-text m-big

    :link-flat-big:`Look here, this is great! <https://mcss.mosra.cz>`

`GitHub`_
=========

For Pelican, download the `m/gh.py <{filename}/plugins.rst>`_ file, put it
including the ``m/`` directory into one of your :py:`PLUGIN_PATHS` and add
:py:`m.gh` package to your :py:`PLUGINS` in ``pelicanconf.py``. For the Python
doc theme, it's enough to just list it in :py:`PLUGINS`:

.. code:: python

    PLUGINS += ['m.gh']

Use the :rst:`:gh:` interpreted text role for linking. The plugin mimics how
`GitHub Flavored Markdown <https://help.github.com/articles/autolinked-references-and-urls/>`_
parses inter-site links, with some extensions on top. In addition to well-known
references to commits and issues/PRs via ``@`` and ``#``, ``$`` is for linking
to a tree (or file in given tree) and ``^`` is for linking to a tag/release. If
your link target doesn't contain any of these characters and contains more than
one slash, the target is simply prepended with ``https://github.com/``.

Link text is equal to link target for repository, commit and issue/PR links,
otherwise the full expanded URL is used. Similarly to builtin linking
functionality, if you want a custom text for a link, use the
:rst:`:gh:`link text <link-target>`` syntax. It's also possible to add custom
CSS classes by deriving the role and adding the :rst:`:class:` option.

.. code-figure::

    .. code:: rst

        .. role:: gh-flat(gh)
            :class: m-flat

        -   Profile link: :gh:`mosra`
        -   Repository link: :gh:`mosra/m.css`
        -   Commit link: :gh:`mosra/m.css@4d362223f107cffd8731a0ea031f9353a0a2c7c4`
        -   Issue/PR link: :gh:`mosra/magnum#123`
        -   Tree link: :gh:`mosra/m.css$next`
        -   Tag link: :gh:`mosra/magnum^snapshot-2015-05`
        -   File link: :gh:`mosra/m.css$master/css/m-dark.css`
        -   Arbitrary link: :gh:`mosra/magnum/graphs/contributors`
        -   :gh:`Link with custom title <getpelican/pelican>`
        -   Flat link: :gh-flat:`mosra`

    .. role:: gh-flat(gh)
        :class: m-flat

    -   Profile link: :gh:`mosra`
    -   Repository link: :gh:`mosra/m.css`
    -   Commit link: :gh:`mosra/m.css@4d362223f107cffd8731a0ea031f9353a0a2c7c4`
    -   Issue/PR link: :gh:`mosra/magnum#123`
    -   Tree link: :gh:`mosra/m.css$next`
    -   Tag link: :gh:`mosra/magnum^snapshot-2015-05`
    -   File link: :gh:`mosra/m.css$master/css/m-dark.css`
    -   Arbitrary link: :gh:`mosra/magnum/graphs/contributors`
    -   :gh:`Link with custom title <getpelican/pelican>`
    -   Flat link: :gh-flat:`mosra`

`OpenGL functions and extensions`_
==================================

For Pelican, download the `m/gl.py <{filename}/plugins.rst>`_ file, put it
including the ``m/`` directory into one of your :py:`PLUGIN_PATHS` and add
:py:`m.gl` package to your :py:`PLUGINS` in ``pelicanconf.py``. For the Python
doc theme, it's enough to just list it in :py:`PLUGINS`:

.. code:: python

    PLUGINS += ['m.gl']

Use the :rst:`:glfn:` interpreted text role for linking to functions,
:rst:`:glext:` for linking to OpenGL / OpenGL ES extensions, :rst:`:webglext:`
for linking to WebGL extensions and :rst:`:glfnext:` for linking to extension
functions. In the link target the leading ``gl`` prefix of functions and the
leading ``GL_`` prefix of extensions is prepended automatically.

Link text is equal to full function name including the ``gl`` prefix and
``()`` for functions, equal to extension name or equal to extension function
link, including the vendor suffix. For :rst:`:glfn:`, :rst:`:glext:` and
:rst:`:webglext:` it's possible to specify alternate link text using the
well-known syntax. Adding custom CSS classes can be done by deriving the role
and adding the :rst:`:class:` option.

.. code-figure::

    .. code:: rst

        .. role:: glfn-flat(glfn)
            :class: m-flat

        -   Function link: :glfn:`DispatchCompute`
        -   Extension link: :glext:`ARB_direct_state_access`
        -   WebGL extension link: :webglext:`OES_texture_float`
        -   Extension function link: :glfnext:`SpecializeShader <ARB_gl_spirv>`
        -   :glfn:`Custom link title <DrawElementsIndirect>`
        -   Flat link: :glfn-flat:`DrawElements`

    .. role:: glfn-flat(glfn)
        :class: m-flat

    -   Function link: :glfn:`DispatchCompute`
    -   Extension link: :glext:`ARB_direct_state_access`
    -   WebGL extension link: :webglext:`OES_texture_float`
    -   Extension function link: :glfnext:`SpecializeShader <ARB_gl_spirv>`
    -   :glfn:`Custom link title <DrawElementsIndirect>`
    -   Flat link: :glfn-flat:`DrawElements`

`Vulkan functions and extensions`_
==================================

For Pelican, download the `m/vk.py <{filename}/plugins.rst>`_ file, put it
including the ``m/`` directory into one of your :py:`PLUGIN_PATHS` and add
:py:`m.vk` package to your :py:`PLUGINS` in ``pelicanconf.py``. For the Python
doc theme, it's enough to just list it in :py:`PLUGINS`:

.. code:: python

    PLUGINS += ['m.vk']

Use the :rst:`:vkfn:` interpreted text role for linking to functions,
:rst:`:vktype:` for linking to types and :rst:`:vkext:` for linking to
extensions. In the link target the leading ``vk`` prefix of functions, ``Vk``
prefix of types and the leading ``VK_`` prefix of extensions is prepended
automatically.

Link text is equal to full function name including the ``vk`` prefix and
``()`` for functions, ``Vk`` prefix for types or equal to extension name. It's
possible to specify alternate link text using the well-known syntax.

.. code-figure::

    .. code:: rst

        .. role:: vkfn-flat(vkfn)
            :class: m-flat

        -   Function link: :vkfn:`CreateInstance`
        -   Type link: :vktype:`InstanceCreateInfo`
        -   Definition link: :vktype:`VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO <StructureType>`
        -   Extension link: :vkext:`KHR_swapchain`
        -   :vkfn:`Custom link title <DestroyInstance>`
        -   Flat link :vkfn-flat:`DestroyDevice`

    .. role:: vkfn-flat(vkfn)
        :class: m-flat

    -   Function link: :vkfn:`CreateInstance`
    -   Type link: :vktype:`InstanceCreateInfo`
    -   Definition link: :vktype:`VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO <StructureType>`
    -   Extension link: :vkext:`KHR_swapchain`
    -   :vkfn:`Custom link title <DestroyInstance>`
    -   Flat link :vkfn-flat:`DestroyDevice`

`Doxygen documentation`_
========================

For Pelican, download the `m/dox.py <{filename}/plugins.rst>`_ file, put it
including the ``m/`` directory into one of your :py:`PLUGIN_PATHS` and add
:py:`m.dox` package to your plugins in ``pelicanconf.py``. The plugin uses
Doxygen tag files to get a list of linkable symbols and you need to provide
list of tuples containing tag file path, URL prefix, an optional list of
implicitly prepended namespaces and an optional list of CSS classes for each
link in a :py:`M_DOX_TAGFILES` configuration value. Example:

.. code:: python

    PLUGINS += ['m.dox']
    M_DOX_TAGFILES = [
        ('doxygen/stl.tag', 'https://en.cppreference.com/w/'),
        ('doxygen/corrade.tag', 'https://doc.magnum.graphics/corrade/', ['Corrade::']),
        ('doxygen/magnum.tag', 'https://doc.magnum.graphics/magnum/', ['Magnum::'], ['m-text', 'm-flat'])]

For the Python doc theme, the configuration is the same. Tag file paths are
relative to the configuration file location or to :py:`PATH`, if specified.

Use the :rst:`:dox:` interpreted text role for linking to documented symbols.
All link targets understood by Doxygen's ``@ref`` or ``@link`` commands are
understood by this plugin as well, in addition it's possible to link to the
documentation index page by specifying the tag file basename w/o extension as
link target. In order to save you some typing, the leading namespace(s)
mentioned in the :py:`M_DOX_TAGFILES` setting can be omitted when linking to
given symbol.

Link text is equal to link target in all cases except for pages and sections,
where page/section title is extracted from the tagfile. It's possible to
specify custom link title using the :rst:`:dox:`link title <link-target>``
syntax. If a symbol can't be found, a warning is printed to output and link
target is rendered in a monospace font (or, if custom link title is specified,
just the title is rendered, as normal text). You can append ``#anchor`` to
``link-target`` to link to anchors that are not present in the tag file (such
as ``#details`` for the detailed docs or ``#pub-methods`` for jumping straight
to a list of public member functions), the same works for query parameters
starting with ``?``. Adding custom CSS classes can be done by deriving the role
and adding the :rst:`:class:` option.

.. code-figure::

    .. code:: rst

        .. role:: dox-flat(dox)
            :class: m-flat

        -   Function link: :dox:`Utility::Directory::mkpath()`
        -   Class link: :dox:`Interconnect::Emitter`
        -   Page link: :dox:`building-corrade`
        -   :dox:`Custom link title <testsuite>`
        -   :dox:`Link to documentation index page <corrade>`
        -   :dox:`Link to an anchor <Interconnect::Emitter#pub-methods>`
        -   Flat link: :dox-flat:`plugin-management`

    .. role:: dox-flat(dox)
        :class: m-flat

    -   Function link: :dox:`Utility::Directory::mkpath()`
    -   Class link: :dox:`Interconnect::Emitter`
    -   Page link: :dox:`building-corrade`
    -   :dox:`Custom link title <testsuite>`
    -   :dox:`Link to documentation index page <corrade>`
    -   :dox:`Link to an anchor <Interconnect::Emitter#pub-methods>`
    -   Flat link: :dox-flat:`plugin-management`

It's also possible to add custom CSS classes via a fourth tuple item. For
example, to make the links consistent with the Doxygen theme (where
documentation links are not underscored, internal doxygen links are bold and
external not), you could do this:

.. code:: python

    PLUGINS += ['m.dox']
    M_DOX_TAGFILES = [
        ('doxygen/stl.tag', 'https://en.cppreference.com/w/', [],
            ['m-flat']),
        ('doxygen/your-lib.tag', 'https://doc.your-lib.com/', ['YourLib::'],
            ['m-flat', 'm-text', 'm-strong'])]

.. note-success::

    For linking to Sphinx documentation, a similar functionality is provided
    by the `m.sphinx <{filename}/plugins/sphinx.rst>`_ plugin. And if you
    haven't noticed yet, m.css also provides a
    `full-featured Doxygen theme <{filename}/documentation/doxygen.rst>`_ with
    first-class search functionality. Check it out!

`Abbreviations`_
================

While not exactly a link but rather a way to produce correct :html:`<abbr>`
elements, it belongs here as it shares a similar syntax.

For Pelican, download the `m/abbr.py <{filename}/plugins.rst>`_ file, put it
including the ``m/`` directory into one of your :py:`PLUGIN_PATHS` and add
:py:`m.abbr` package to your :py:`PLUGINS` in ``pelicanconf.py``. This plugin
assumes presence of `m.htmlsanity <{filename}/plugins/htmlsanity.rst>`_.

.. code:: python

    PLUGINS += ['m.htmlsanity', 'm.abbr']

The plugin overrides the builtin Pelican
`abbr interpreted text role <https://docs.getpelican.com/en/stable/content.html#file-metadata>`_
and makes its syntax consistent with other common roles of :abbr:`reST <reStructuredText>`
and m.css.

Use the :rst:`:abbr:` interpreted text role for creating abbreviations with
title in angle brackets. Adding custom CSS classes can be done by deriving the
role and adding the :rst:`:class:` option.

.. code-figure::

    .. code:: rst

        .. role:: abbr-warning(abbr)
            :class: m-text m-warning

        :abbr:`HTML <HyperText Markup Language>` and :abbr-warning:`CSS <Cascading Style Sheets>`
        are *all you need* for producing rich content-oriented websites.

    .. role:: abbr-warning(abbr)
        :class: m-text m-warning

    :abbr:`HTML <HyperText Markup Language>` and :abbr-warning:`CSS <Cascading Style Sheets>`
    are *all you need* for producing rich content-oriented websites.

`File size queries`_
====================

Okay, this is not a link at all, but --- sometimes you might want to display
size of a file, for example to tell the users how big the download will be.

For Pelican, ownload the `m/filesize.py <{filename}/plugins.rst>`_ file, put it
including the ``m/`` directory into one of your :py:`PLUGIN_PATHS` and add
:py:`m.filesize` package to your :py:`PLUGINS` in ``pelicanconf.py``.

.. code:: python

    PLUGINS += ['m.filesize']

Use the :rst:`filesize` interpreted text role to display the size of a file
including units. The :rst:`filesize-gz` role compresses the file using GZip
first before calculating the size. Adding custom CSS classes can be done by
deriving the role and adding the :rst:`:class:` option.

.. code-figure::

    .. code:: rst

        .. role:: filesize-yay(filesize-gz)
            :class: m-text m-success

        The compiled ``m-dark.compiled.css`` CSS file has
        :filesize:`{static}/../css/m-dark.compiled.css` but only
        :filesize-yay:`{static}/../css/m-dark.compiled.css` when the server
        sends it compressed.

    .. role:: filesize-yay(filesize-gz)
        :class: m-text m-success

    The compiled ``m-dark.compiled.css`` CSS file has
    :filesize:`{static}/../css/m-dark.compiled.css` but only
    :filesize-yay:`{static}/../css/m-dark.compiled.css` when the server
    sends it compressed.

`Aliases`_
==========

Site content almost never stays on the same place for extended periods of time
and preserving old links for backwards compatibility is a vital thing for user
friendliness. This plugin allows you to create a redirect alias URLs for your
pages and articles.

For Pelican, download the `m/alias.py <{filename}/plugins.rst>`_ file, put it
including the ``m/`` directory into one of your :py:`PLUGIN_PATHS` and add
:py:`m.alias` package to your :py:`PLUGINS` in ``pelicanconf.py``.

.. code:: python

    PLUGINS += ['m.alias']

.. note-success::

    This plugin is loosely inspired by :gh:`Nitron/pelican-alias`, © 2013
    Christopher Williams, licensed under
    :gh:`MIT <Nitron/pelican-alias$master/LICENSE.txt>`.

Use the :rst:`:alias:` field to specify one or more locations that should
redirect to your article / page. Each line is treated as one alias, the
locations have to begin with ``/`` and are relative to the Pelican output
directory, each of them contains just a :html:`<meta http-equiv="refresh" />`
that points to a fully-qualified URL of the article or page.

If the alias ends with ``/``, the redirector file is saved into ``index.html``
in given directory.

.. code:: rst

    My Article
    ##########

    :alias:
        /2018/05/06/old-version-of-the-article/
        /even-older-version-of-the-article.html
