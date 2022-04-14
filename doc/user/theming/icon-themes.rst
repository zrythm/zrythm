.. SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. _icon-themes:

Icon Themes
===========
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
