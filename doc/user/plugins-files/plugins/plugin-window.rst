.. SPDX-FileCopyrightText: © 2019-2020, 2022 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

Plugin Window
=============

Custom UIs
----------
When Plugin UIs are opened, a window such
as the following will be displayed, if the
plugin ships with its own UI. This window may
contain additions by Zrythm (such as
`File` and `Presets` menus).

.. image:: /_static/img/della_plugin_window.png
   :align: center

Generic UIs
-----------
If the plugin does not ship with its own UI,
the following generic UI will be generated
for it.

.. image:: /_static/img/haas_plugin_window.png
   :align: center

Bridging
--------
Most plugins are instantiated in the same process
as Zrythm. This has the advantage that :term:`DSP`
is very fast, however, some plugin UIs are not
compatible with Zrythm
and need to be bridged (ie, run as separate
processes).

.. note:: When it is possible to bridge only the UI,
  Zrythm will do so, and this will not have any
  performance implications on DSP. If bridging the whole
  plugin is required, this will have some performance
  implications, since it is required to communicate
  with the plugin in another process.

This process is transparent to the user and only
mentioned for reference.

Opening Plugins in Bridged Mode
-------------------------------
Sometimes, you may need to manually tell Zrythm
to open plugins in a specific way. You can find the
options available for each plugin by right clicking
on the plugin in the :ref:`plugin-browser`.

.. figure:: /_static/img/open-plugin-bridged.png
   :align: center

   Opening ZynAddSubFX in bridged mode
