.. This is part of the Zrythm Manual.
   Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _edit-modes:

Edit Modes
==========

When working in each editor or arranger, you will most likely be
using the cursor tools provided by Zrythm to create,
edit and delete objects. Each cursor signifies a separate
edit mode, and Zrythm offers the following edit modes.

.. image:: /_static/img/toolbox.png
   :align: center

Select/Stretch Mode
~~~~~~~~~~~~~~~~~~~
This can be toggled to switch between Select and Stretch.

In Select mode, you can make object selections and move,
clone and resize objects. This is the most common mode
and the most common operations can be accomplished by just
using this.

Stretch mode is similar to the Select mode, with the
exception that when you resize objects, their contents
will be stretched to fit the new size.

Selecting Objects
+++++++++++++++++
To select objects, you can either click on them, click
in empty space and keep dragging to make a selection, or
:zbutton:`Control`-click to add/remove objects to/from the
selection.

Selected objects will have brighter color than non-selected
objects.

Moving and Cloning Objects
++++++++++++++++++++++++++
To move objects around, click on a selected object and keep
dragging with your cursor to where you want to move them.
Holding down :zbutton:`Control` while doing this will create
duplicates (clones) of the selected objects. This is an
efficient way to quickly clone multiple objects.

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
~~~~~~~~~~~~~~~
You can :zbutton:`Alt`-click inside objects to break them up.
Holding down :zbutton:`Shift` while doing this will bypass
snapping.

Edit Mode
~~~~~~~~~
This mode is used to quickly create or delete objects.
While you can still create objects in Select mode by
double clicking and dragging, in Edit mode this is
accomplished by single clicking and dragging, which is
more efficient when creating a large number of objects.

Holding down :zbutton:`Control` while clicking will allow
you to create multiple objects in a row (where applicable)
with a length specified by the snap settings corresponding
to each editor.

Cut Mode
~~~~~~~~
Cut mode behaves similarly to what is mentioned in
:ref:`cutting-objects`, with the exception that you
can just click instead of :zbutton:`Alt`-clicking.

Erase Mode
~~~~~~~~~~
You can just click on objects to delete them, or click
and drag to make a selection, deleting all objects inside it.

.. _ramp-mode:

Ramp Mode
~~~~~~~~~
This mode is currently only used for editing velocities for
MIDI notes. You can click and drag in the velocity editor
to create velocity ramps.

.. image:: /_static/img/ramp-tool.png
   :align: center

Audition Mode
~~~~~~~~~~~~~
This is a TODO feature.
