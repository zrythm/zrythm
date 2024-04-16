.. SPDX-FileCopyrightText: © 2020, 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
.. SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

Edit Tools
==========

When working in each editor or arranger, you will most
likely be using the cursor tools provided by Zrythm
to create, edit and delete objects. Each cursor
signifies a separate tool, and Zrythm offers the
following tools.

.. image:: /_static/img/toolbox.png
   :align: center

Select Tool
~~~~~~~~~~~
In *select* mode, you can make object selections and
create, move, clone and resize objects. This is the
most common mode and the most common operations can
be accomplished by just using this.

Creating Objects
++++++++++++++++
Using the select (default) tool, all objects are
created by double clicking inside their
corresponding arranger and dragging, then releasing
when you are satisfied with the position/size.

.. figure:: /_static/img/creating-midi-region.png
   :align: center

   Creating a region

Selecting Objects
+++++++++++++++++
To select objects, you can either click on them, click
in empty space and keep dragging to make a selection, or
:kbd:`Control`-click to add/remove objects to/from the
selection.

Selected objects will have brighter color than non-selected
objects.

.. figure:: /_static/img/object-selection.png
   :align: center

   Selecting objects

Moving Objects
++++++++++++++
Objects are moved by clicking and dragging them
around. You can move regions to other tracks if
the track types are compatible.

.. figure:: /_static/img/moving-midi-region.png
   :align: center

   Moving a region

Cloning (Copy-Moving) Objects
+++++++++++++++++++++++++++++

Holding down :kbd:`Ctrl` while moving objects will
allow you to copy-and-move the objects to the new
location.

.. figure:: /_static/img/copy-moving-midi-region.png
   :align: center

   Copy-moving a region

Linking (Link-Moving) Objects
+++++++++++++++++++++++++++++
Regions can be linked by holding down :kbd:`Alt` while moving.
Changes made in a linked region will be propagated to all other regions in the same link group.

.. tip:: This is useful for re-using the exact same regions across a song without having to re-copy the regions when new changes are made.

.. figure:: /_static/img/link-moving-regions.png
   :align: center

   Link-moving a MIDI region

Linked regions will display a link icon in their top-right edge.

.. figure:: /_static/img/linked-regions.png
   :align: center

   Linked regions

Resizing Objects
++++++++++++++++
Objects that have length, such as regions and MIDI notes,
can be resized by clicking and dragging their edges.

.. figure:: /_static/img/resizing-objects.png
   :align: center

   Resizing a region

Repeating Objects
+++++++++++++++++

Regions can be repeated (looped) by moving the cursor
on the bottom half of the object, then clicking and dragging.

.. figure:: /_static/img/repeating-objects.png
   :align: center

   Repeating a region

Regions that are repeated will display dashed lines at points where looping
occurs, and a loop icon in their top right corner.

.. figure:: /_static/img/repeated-object.png
   :align: center

   A repeated region

The position where the region will initially start playback (the *clip start position*) and the start/end loop points, can be :ref:`adjusted in the clip editor <editing/timeline:Editor Ruler Indicators>`.

.. warning:: If the region is already repeated, it
   cannot be resized anymore until its loop points
   match exactly the region's start and end points.

Stretching Objects
++++++++++++++++++

To stretch one or more regions, hold :kbd:`Control` while hovering the cursor
at a top edge of a region, then click and drag.

.. note:: Only *non-repeated* objects can be stretched.

.. _cutting-objects:

Cutting Objects
+++++++++++++++
You can :kbd:`Alt`-click inside objects to break them up.

.. figure:: /_static/img/cutting-objects.png
   :align: center

   Cutting a region

Erasing Objects
+++++++++++++++
Clicking and holding the right mouse button while hovering over objects in *select* mode will delete them.

Edit Tool
~~~~~~~~~
This tool is used to quickly create or delete objects.
While you can still create objects using the Select tool by
double clicking and dragging, with the Edit tool this is
accomplished by single clicking and dragging, which is
more efficient when creating a large number of objects.

Brush Mode
++++++++++

Holding down :kbd:`Control` while clicking will allow
you to create multiple objects in a row (where applicable)
with a length specified by the snap settings corresponding
to each editor.

.. figure:: /_static/img/edit-brush-mode.png
   :align: center

   Bulk-creating MIDI notes

Cut Tool
~~~~~~~~
The Cut tool behaves similarly to what is mentioned in
:ref:`cutting-objects`, with the exception that you
can just click instead of :kbd:`Alt`-clicking.

Erase Tool
~~~~~~~~~~
Using the Erase tool you can just click on objects to
delete them, or click and drag to make a selection,
deleting all objects inside it.

.. _ramp-mode:

Ramp Tool
~~~~~~~~~
This tool is currently only used for editing
velocities for MIDI notes. You can click and drag in
the velocity editor to create velocity ramps.

.. figure:: /_static/img/ramp-tool.png
   :align: center

   Using the ramp tool on velocities

Audition Tool
~~~~~~~~~~~~~

The Audition tool is used to listen to specific
parts of the song quickly. After enabling the
audition tool, click anywhere to start playback
from the position of the cursor and release the
mouse button to stop.

.. figure:: /_static/img/audition-tool.png
   :align: center

   Using the audition tool
