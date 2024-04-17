.. SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. _making-connections:

Connections
===========

Making Connections
------------------

Direct Connections
~~~~~~~~~~~~~~~~~~

Zrythm allows the user to connect almost any port
to any other port, as long as they are compatible.
For example, you can route an LFO's CV output to
a filter plugin's cutoff parameter.

Connections can be made by double clicking on ports
in the :ref:`track-inspector` or the
:ref:`plugin-inspector-page`.

.. figure:: /_static/img/plugin-controls.png
   :align: center

   Plugin controls

In the popover, you can view existing connections
and press :guilabel:`Add` to create a new connection.

.. figure:: /_static/img/edit-port-connection.png
   :align: center

   Port connections for control

You can select the port to connect to in the
presented dialog.

.. figure:: /_static/img/creating-port-connection.png
   :align: center

   Selecting a new port to connect to

Sidechaining
~~~~~~~~~~~~

Signal from tracks can be routed to plugins that
have sidechain inputs via
:ref:`Aux Sends <tracks/inspector-page:Aux Sends>`.

.. figure:: /_static/img/sidechain-send.png
   :align: center

   Selecting a new port to connect to

For example, you can use a compressor plugin on a
bass track that accepts a sidechain input from a
kick track to get a pumping/ducking effect.

Managing Connections
--------------------

Connections can be managed in the
Connections tab in the
:ref:`main manel <zrythm-interface/main-panel:Main Panel>`.

.. figure:: /_static/img/connections-tab.png
   :align: center

   Managing connections

Individual connections can be edited in the inspector
pages by double clicking.

.. figure:: /_static/img/edit-port-connection.png
   :align: center

   Editing a connection

You can click and drag the bar to select how much
the connection should affect the signal. 100% means
the whole signal will be passed from the source port
to the destination port, and 0% means no signal will
be allowed through.

The following controls are also available.

Enable/Disable Connection
  Toggle whether the connection is enabled (active)
Delete Connection
  Delete the connection

Routing Graph
-------------
It may be helpful to be able to view the routing
graph when making connections. We plan to add this
functionality into Zrythm in the future, but for
now Zrythm allows you to export the whole routing
graph as an image, so you can view it externally.

See :ref:`export-routing-graph` for details.

.. warning:: This functionality is experimental.
