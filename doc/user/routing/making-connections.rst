.. This is part of the Zrythm Manual.
   Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _making-connections:

Making Connections
==================

Zrythm allows the user to connect almost any port
to any other port, as long as they are compatible.
For example, you can route an LFO's CV output to
a filter plugin's cutoff parameter.

Connections can be made by double clicking on ports
in the :ref:`track-inspector` or the
:ref:`plugin-inspector-page`.

.. image:: /_static/img/port_connections.png
   :align: center

By double clicking the port, you can select a
port to connect it to, or edit existing connections.
You can also drag the slider to adjust the level
of the amount to send.

Routing Graph
-------------
It is very helpful to be able to view the routing
graph when making connections. We plan to add this
functionality into Zrythm in the future, but for
now Zrythm allows you to export the whole routing
graph as a PNG image, so you can view it externally.

See :ref:`export-routing-graph` for details.
