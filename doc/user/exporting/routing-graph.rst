.. SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
   This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. _export-routing-graph:

Routing Graph
=============
Zrythm allows you to export its routing graph. This
is useful for debugging and also helps visualizing
the internals of Zrythm.

To export the routing graph, Select the
:guilabel:`Export Graph` option in the hamburger menu in the
:doc:`../zrythm-interface/main-toolbar`. The routing graph
can be exported as either a PNG image or a .dot graph.

.. image:: /_static/img/export-graph-dialog.png
   :align: center

.. note:: For large projects, it may take a while to generate
  the PNG graph and the resulting image will be extremely
  large.

.. warning:: This functionality is experimental.
