.. SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

Theming
=======

Overview
--------

.. important:: Zrythm aims to allow extensive theming support, however it is considered experimental at this stage.

You can override many defaults that Zrythm uses by
placing things in your :ref:`user path <configuration/preferences:Zrythm User Path>`.

The `Zrythm installation directory` refers to where
Zrythm was installed in. This is normally
:file:`/opt/zrythm-<your version>` on GNU/Linux and MacOS, and
:file:`C:\\Program Files\\Zrythm` on Windows.

.. :file:`Resources` under the the Application contents on MacOS.

CSS
---

You can override the GTK theme that Zrythm uses along with
much of the color scheme in a special CSS file. It is
a good idea to start by copying the theme from your Zrythm
installation directory
(normally :file:`/usr/share/zrythm/themes/zrythm-theme.css` on
GNU/Linux and :file:`share/zrythm/themes/zrythm-theme.css` in
the installation path on Windows or the Application contents
on MacOS.

You should create a file called :file:`theme.css` under the
:file:`themes` directory in your Zrythm directory. For example,
if your Zrythm directory is :file:`/home/me/zrythm`, you should
create a file at :file:`/home/me/zrythm/themes/theme.css`.

.. warning:: This is highly experimental and many things are subject to change.

GTK-Specific Rules
~~~~~~~~~~~~~~~~~~
You will find lots of information about changing CSS
rules for general GTK widgets online.

Here are some links by official GNOME sources:

* `GTK+ CSS Overview <https://docs.gtk.org/gtk4/css-overview.html>`_
* `GTK+ CSS Properties <https://docs.gtk.org/gtk4/css-properties.html>`_
* `Styling GTK with CSS <https://thegnomejournal.wordpress.com/2011/03/15/styling-gtk-with-css/>`_

Icon Themes
-----------

.. important:: Icon theming is deprecated and will not be supported until we figure out a nice way to do it.
   The following text still applies to some extent, however most icons used inside Zrythm are bundled inside the Zrythm executable and are not themable at the moment.

   Moreover, the following functionality is inherited from GTK, however
   it is likely that GTK/Freedesktop will change how icon theming works in the future, so we only mention it for reference.

The icon theme that Zrythm uses is freedesktop-compliant,
so it can be easily overridden.

To override the Zrythm icon theme with your own icon theme,
you need to create a directory similar to
:file:`share/zrythm/themes/icons/zrythm-dark`
under your Zrythm installation directory.

For example,
if your Zrythm directory is :file:`/home/me/zrythm`, you should
create a directory at
:file:`/home/me/zrythm/themes/icons/zrythm-dark` with an
:file:`index.theme` file and the icons you wish to override.

An easy way to do it is by copying the directory that Zrythm
comes with and replacing the icons with your own.

A more elegant way to do it is by following the
`Freedesktop Icon Theme Specification <https://specifications.freedesktop.org/icon-theme-spec/latest/>`_.
