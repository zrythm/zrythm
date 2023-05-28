.. SPDX-FileCopyrightText: © 2023 Cris Cao <qygw@outlook.com>
   SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

UI Scaling and Wayland
======================

About Wayland
-------------
Zrythm currently forces X11 (i.e., XWayland) when running on
Wayland because almost all plugins on GNU/Linux are built
against X11 and some plugin UIs crash when running under
Wayland directly.

.. hint:: You can avoid these crashes by :ref:`bridging<plugins-files/plugins/plugin-window:Bridging>` plugins.

If you are fine with this and prefer Zrythm to use pure Wayland
you can use the :envvar:`GDK_BACKEND` environment variable.
You can either pass this variable when running Zrythm from the
terminal,

.. code-block:: bash

   $ export GDK_BACKEND=wayland
   $ zrythm_launch

or you can edit the Desktop Entry for Zrythm.

.. code-block:: diff

   - Exec=/usr/bin/zrythm_launch
   + Exec=env GDK_BACKEND=wayland /usr/bin/zrythm_launch

.. seealso::
   `Gtk - 4.0: Using GTK with Wayland <https://docs.gtk.org/gtk4/wayland.html>`_

UI Scaling
----------
If you are using Wayland and your window manager already has the
correct scaling settings, you may be able to get the user
interface scaled correctly by having Zrythm running entirely in
Wayland using :envvar:`GDK_BACKEND`.

On Xorg, you can scale the interface by setting the
:envvar:`GDK_SCALE` environment variable:

.. code-block:: bash

   $ export GDK_SCALE=2
   $ zrythm_launch

.. attention:: GTK3/4 does not support fractional scaling
   currently, so fractional factors will be ignored. This
   environment variable is also ignored in Mutter Wayland
   sessions.
