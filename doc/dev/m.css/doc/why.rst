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

Why?
####

Let's start with some history. Back in 2002, when I star---

Eh, everybody already heard this. Old man yells at cloud. Complaining that
before a webpage had 50 kB, now it has 5 MB, still taking *ten seconds* to
load, even though the connections are hundreds times faster. But why is that?
Because *nobody cares*.

I am a GPU developer. My usual Mondays are about having a handful of
milliseconds to do complex image processing in real time. So I am furious if a
page that presents mainly *text* takes *seconds* to appear on my screen. And I
would despise myself for presenting GPU-related content using such tools. So I
made this thing. This thing has no JavaScript, just a few kilobytes of pure CSS
and is meant to be used with static site generators.

.. note-danger:: Warning: opinions

    This page presents my opinions. Not everybody likes my opinions.

Why no JavaScript?
==================

Responsive page layout? Yes! Popup menus? Check! Expanding hamburger
navigation? Sure! With pure CSS features, supported *for years* by all
browsers. The goal of this framework is presenting *content* in the most
unobtrusive and lightweight way possible --- if you are looking for something
that would display a spinner while *the megabytes* are loading, something that
can open a "SIGN UP" popup on page load or interrupt the reading experience
with a noisy full-page ad every once a while, this is not what you are looking
for.

.. note-info::

    Well, while m.css itself doesn't *need* JavaScript, it doesn't prevent
    *you* from using it --- put in as many jQueries as you want.

Why no CSS preprocessors?
=========================

The stylesheets consist of only `three files <{filename}/css.rst>`_, written in
pure CSS. No CSS preprocessor needed --- in my experience the main reason for a
preprocessor was an ability to use variables, but that's implemented in CSS
itself with universal support across browsers, so why bother? Besides that, the
files are so small that there's no need for any minification --- server-side
compression is simply good enough.

.. note-dim::

    Actually, in order to support old Edge versions and IE, I *had to* create a
    *post*\ processed versions of the CSS files. But that still doesn't prevent
    you from using pure CSS.

Why a static site generator?
============================

If you are like me, you want a website that presents your main work, not that
your main work is presenting websites. So remove the unneeded hassle of paying
extra for a dynamic webhosting, installing and maintaining a huge CMS and
patching it twice a month for a neverending stream new CVEs. Together with
`Pelican <{filename}/themes/pelican.rst>`_, m.css provides a solution
harnessing the endless possibilities of your local machine to generate a bunch
of static HTML files, which you can then upload *anywhere*. And as a bonus, you
have the full control over your content --- nothing's hidden in opaque
databases.

.. note-success::

    This is all optional, though --- m.css can work with a dynamic CMS below as
    well. Or with your hand-written HTML. Not sure about WYSIWYG editors, tho.

Why Pelican and not Jekyll or ...?
==================================

I *love* Python and while I'm probably not very good at it (having spent over a
decade writing purely C++), I wanted to finally put it into practical use,
instead of learning a completely new language. There I also discovered
reStructuredText, which, compared to various flavors of Markdown, is very well
thought out, amazingly flexible and easily extensible to support
`everything I need <{filename}/plugins.rst>`_.

.. note-warning::

    By all means, use Jekyll or Hugo, if you want --- but I can't promise that
    all the :abbr:`reST <reStructuredText>` extensions I did are transferrable
    elsewhere.

Why the name?
=============

To underline that this project is driven by a need for a simple, tiny,
nameless low-maintenance thing to power my websites. That my focus is on
presenting content with it, not on shifting my career back to web development
to flesh this thing out to be the Greatest Web Framework Ever™.

.. note-primary::

    Contributions welcome, though! Does it look like I'm not putting enough
    effort in? Submit an improvement! Make a difference
    :gh:`over at GitHub <mosra/m.css/issues/new>`. Both the m.css code and
    contents of this site are public, available under the MIT license.
