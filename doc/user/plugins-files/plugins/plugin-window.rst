.. SPDX-FileCopyrightText: © 2019-2020, 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

Plugin Window
=============

Custom UIs
----------
When Plugin UIs are opened, a window such as the following
will be displayed, if the plugin ships with its own UI.

.. image:: /_static/img/ot-simian-plugin-window.png
   :align: center

Generic UIs
-----------
If the plugin does not ship with its own UI, the following
generic UI will be generated for it.

.. image:: /_static/img/compressor-plugin-window.png
   :align: center

Bridging
--------

By default, plugins are instantiated in a separate process from Zrythm so that Zrythm
doesn't crash if a plugin misbehaves (Zrythm calls this *bridging*). However, this has the side effect that
:term:`DSP` is not as fast as it could be (although fast enough in most
cases), because there is cross-process communication.

Sometimes, you may want to manually tell Zrythm
to open plugins in a specific way. You can find the
options available for each plugin by right clicking
on the plugin in the :ref:`plugin-browser`.

.. figure:: /_static/img/plugin-browser-right-click.png
   :align: center

   Opening Compressor in normal (unbridged) mode

.. tip:: Zrythm will remember the last setting used for each
   plugin and automatically apply it when you choose to
   instantiate that plugin from the plugin browser in the future.
