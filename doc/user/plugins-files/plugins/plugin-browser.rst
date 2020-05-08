.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _plugin-browser:

Plugin Browser
==============

The Plugin Browser makes it easy to browse and
filter Plugins installed on your computer.

.. image:: /_static/img/plugin_browser.png
   :align: center

Filters Tab
-----------
The filters tab at the top allows you to select how to filter
the Plugins. There are 3 tabs:

Collection
  This tab contains your collections. You can create
  collections such as "MySynths" and filter by the selected
  collections.
Category
  Filter Plugins by category based on the plugin metadata,
  such as "Delay", "Distortion", etc.
Protocol
  Allows you to filter plugins based their protocol (LV2 or
  VST).

.. note:: Collections and Protocols are not operational at the moment.

Filter Buttons
--------------
Additionally to the above, you can filter plugins based on
their type. See :ref:`plugin-types` for more information
about the various types of plugins there are.

.. _instantiating-plugins:

Instantiating Plugins
---------------------
There are a couple of ways to instantiate plugins:

Drag-n-Drop
~~~~~~~~~~~

You can drag and drop the selected plugin into empty space in the
Tracklist or into empty space in the Mixer to
create a new track using that plugin.

.. image:: /_static/img/drop-plugin-to-mixer.png
   :align: center

.. image:: /_static/img/drop-plugin-to-tracklist.png
   :align: center

Alternatively, you can drag the plugin on a mixer slot
to add it there or replace the previous plugin.

.. image:: /_static/img/drop-plugin-to-slot.png
   :align: center

If the plugin is a
modulator, you can drop it into the Modulators Tab.

.. note::
  The Modulators tab is a work-in-progress. For the time being,
  you can use them as regular effects.

Double Click/Enter
~~~~~~~~~~~~~~~~~~

Double click on the plugin or select it and press the
return key on your keyboard to create a new track using
that plugin.
