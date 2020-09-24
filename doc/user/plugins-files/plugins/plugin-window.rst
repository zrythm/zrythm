.. This is part of the Zrythm Manual.
   Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Plugin Window
=============

Custom UIs
----------
When Plugin UIs are opened, a window such
as the following will be displayed, if the
plugin ships with its own UI. This window will
contain additions by Zrythm (such as the
`File` and `Presets` menus).

.. image:: /_static/img/della_plugin_window.png
   :align: center

.. note:: Some plugin UIs cannot be wrapped and will
   be opened in separate windows without additions
   like `File` or `Presets`.

Generic UIs
-----------
If the plugin does not ship with its own UI,
the following generic UI will be generated
for it.

.. image:: /_static/img/haas_plugin_window.png
   :align: center

.. note:: This only applies to :term:`LV2` plugins.
   For other types of plugins, you can edit their
   parameters in the plugin inspector page (see
   :ref:`plugin-inspector-page`).

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

.. image:: /_static/img/open-plugin-bridged.png
