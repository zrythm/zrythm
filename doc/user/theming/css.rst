.. SPDX-FileCopyrightText: © 2020, 2022 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. _css:

CSS
===
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

Color Scheme
------------
Inside the theme file, you will find lots of colors defined
using ``@define-color``. You can override these with your
own colors and Zrythm will use them where possible.

Other GTK-Specific Rules
------------------------
You will find lots of information about changing CSS
rules for general GTK widgets online.

Here are some links by official GNOME sources:

* `GTK+ CSS Overview <https://docs.gtk.org/gtk4/css-overview.html>`_
* `GTK+ CSS Properties <https://docs.gtk.org/gtk4/css-properties.html>`_
* `Styling GTK with CSS <https://thegnomejournal.wordpress.com/2011/03/15/styling-gtk-with-css/>`_
