.. SPDX-FileCopyrightText: © 2020, 2022 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
   This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. _edit-tools:

Edit Tools
==========

When working in each editor or arranger, you will most
likely be using the cursor tools provided by Zrythm
to create, edit and delete objects. Each cursor
signifies a separate tool, and Zrythm offers the
following tools.

.. image:: /_static/img/toolbox.png
   :align: center

Select/Stretch Tool
~~~~~~~~~~~~~~~~~~~
This can be toggled to switch between Select and
Stretch.

In Select mode, you can make object selections and
create, move, clone and resize objects. This is the
most common mode and the most common operations can
be accomplished by just using this.

Stretch mode is similar to the Select mode, with the
exception that when you resize objects, their contents
will be stretched to fit the new size.

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

Resizing Objects
++++++++++++++++
Objects that have length, such as regions and MIDI notes,
can be resized by clicking and dragging their edges.

On objects that can be repeated, such as regions, moving
the cursor on the bottom half of the object will allow you
to resize the object by repeating it, instead of just
resizing it.

.. note:: When resizing, already repeated objects can only be
   repeated. To resize them instead of repeat them,
   their length must be changed or their clip markers must be
   moved so that they are not repeated anymore.

.. _cutting-objects:

Cutting Objects
+++++++++++++++
You can :kbd:`Alt`-click inside objects to break
them up.

Edit Tool
~~~~~~~~~
This tool is used to quickly create or delete objects.
While you can still create objects using the Select tool by
double clicking and dragging, with the Edit tool this is
accomplished by single clicking and dragging, which is
more efficient when creating a large number of objects.

Holding down :kbd:`Control` while clicking will allow
you to create multiple objects in a row (where applicable)
with a length specified by the snap settings corresponding
to each editor.

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

.. tip:: Where applicable, hold :kbd:`Shift` to
   bypass snapping.

.. tip:: Pressing :kbd:`Escape` will cancel any
   current action.
