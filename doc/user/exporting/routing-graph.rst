.. This is part of the Zrythm Manual.
   Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _export-routing-graph:

Routing Graph
=============
Zrythm allows you to export its routing graph. This
is useful for debugging and also helps visualizing
the internals of Zrythm.

Clicking the
:zbutton:`Export Graph` button will
show the following selection. The routing graph
can be exported as either a PNG image or a .dot graph.

.. image:: /_static/img/export-graph-dialog.png
   :align: center

.. note:: For large projects, it may take a while to generate
  the PNG graph and the resulting image will be extremely
  large.
