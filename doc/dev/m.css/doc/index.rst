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

m.css
#####

:save_as: index.html
:url:
:cover: {static}/static/cover.jpg
:summary: A no-nonsense, no-JavaScript CSS framework and Pelican theme for
    content-oriented websites
:hide_navbar_brand: True
:landing:
    .. container:: m-row

        .. container:: m-col-l-6 m-push-l-1 m-col-m-7 m-nopadb

            .. raw:: html

                <h1>m<span class="m-thin">.css</span></h1>

    .. container:: m-row

        .. container:: m-col-l-6 m-push-l-1 m-col-m-7 m-nopadt

            *A no-nonsense, no-JavaScript CSS framework and Pelican theme for
            content-oriented websites.*

            Do you *hate* contemporary web development like I do? Do you also
            feel that it's not right for a web page to take *seconds* and
            *megabytes* to render? Do you want to write beautiful content but
            *can't* because the usual CMS tools make your blood boil and so you
            rather stay silent? Well, I have something for you.

        .. container:: m-col-l-3 m-push-l-2 m-col-m-4 m-push-m-1 m-col-s-6 m-push-s-3 m-col-t-8 m-push-t-2

            .. button-primary:: https://github.com/mosra/m.css/tree/master/css/m-dark.compiled.css
                :class: m-fullwidth

                Get the essence

                | :filesize-gz:`{static}/../css/m-dark.compiled.css` of gzipped CSS,
                | licensed under MIT

.. container:: m-row m-container-inflate

    .. container:: m-col-m-4

        .. block-success:: *Pure* CSS and HTML

            Everything you need is :filesize-gz:`{static}/../css/m-dark.compiled.css`
            of compressed CSS. This framework has exactly 0 bytes of JavaScript
            because *nobody actually needs it*. Even for responsive websites.

            .. button-success:: {filename}/css.rst
                :class: m-fullwidth

                Get the CSS

    .. container:: m-col-m-4

        .. block-warning:: Designed for *content*

            If you just want to write content with beautiful typography, you
            don't need forms, progressbars, popups, dropdowns or other cruft.
            You want fast iteration times.

            .. button-warning:: {filename}/themes/pelican.rst
                :class: m-fullwidth

                Use it with Pelican

    .. container:: m-col-m-4

        .. block-info:: Authoring made *easy*

            Code snippets, math, linking to docs, presenting photography in a
            beautiful way? Or making a complex page without even needing to
            touch HTML? Everything is possible.

            .. button-info:: {filename}/plugins.rst
                :class: m-fullwidth

                Get Pelican plugins

.. class:: m-text-center m-noindent

*Still not convinced?* Head over to a `detailed explanation <{filename}/why.rst>`_
of this project goals and design decisions.
