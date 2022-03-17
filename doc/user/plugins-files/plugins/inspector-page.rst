.. This is part of the Zrythm Manual.
   Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _plugin-inspector-page:

Inspector Page
==============

When a plugin is selected in the Mixer, or when using
right click -> Inspect on an instrument, its
page will appear in the Inspector as follows.

.. figure:: /_static/img/plugin-inspector.png
   :align: center

   Plugin inspector

This page will display information about the
plugin and its ports and allow you to make
changes.

Plugin Properties
-----------------
The plugin properties show the name and type of the plugin,
as well as preset and bank selectors.

.. image:: /_static/img/plugin-properties.png
   :align: center

Plugin Ports
------------
These are the ports that the plugin has.

.. image:: /_static/img/plugin-ports.png
   :align: center

Control input ports can be changed by clicking and
dragging, and can also be automated using
automation tracks.

Ports can be routed to other ports anywhere
within Zrythm by double clicking on them.

Ports are explained in more detail in :ref:`ports`.

.. note:: Depending on the type of plugin, there may
   be different categories of ports being shown.

Display
~~~~~~~
The number in the top right of each port name will
display the current number of connections to/from
this port. See
:doc:`../../routing/making-connections` for details.

Changing Values
~~~~~~~~~~~~~~~
The values of control ports can be changed by
clicking and dragging inside each bar.

.. seealso:: :ref:`plugins-files/plugins/plugin-info:Plugin Ports`
