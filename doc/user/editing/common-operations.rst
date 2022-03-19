.. This is part of the Zrythm Manual.
   Copyright (C) 2020, 2022 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _common-operations:

Common Operations
=================

These operations are common to most editors, and behave
similarly to what you should be used to from other
applications.

Undo/Redo
---------
Most actions in Zrythm are undoable, which means
that you can go back to the state before that
action was made, or you can come back to a state
that you undo'ed from.

For example, if you create a MIDI note and press
undo, the MIDI note will be deleted (Zrythm goes
back into the state where the MIDI note did not
exist). Pressing the :guilabel:`Redo` button will
create the note again, and you can go back between
:guilabel:`Undo` and :guilabel:`Redo` as many times
as you want.

It is also possible to go back/forth more than one
action, by clicking the arrow next to the
:guilabel:`Undo` and :guilabel:`Redo` buttons.

.. figure:: /_static/img/undo-multiple.png
   :align: center

   Undoing multiple actions

The shortcuts for undo and redo are
:kbd:`Control-z` and :kbd:`Control-Shift-z`.

.. warning:: If you Undo and then perform a new
   action, the Redo history will be deleted.

Object Operations
-----------------
Cut
~~~
Delete and copy the selected objects and save them in
the clipboard so that they can be pasted elsewhere.
The shortcut for cutting is
:kbd:`Control-x`.

Copy
~~~~
Copy the selected objects to the clipboard so that they
can be pasted elsewhere.
The shortcut for copying is
:kbd:`Control-c`.

Paste
~~~~~
Paste the objects currently in the clipboard to the current
playhead position.
The shortcut for pasting is
:kbd:`Control-v`.

Duplicate
~~~~~~~~~
Create a copy of the selected objects adjacent to
them. The shortcut for duplicating is
:kbd:`Control-d`.

.. figure:: /_static/img/duplicating-midi-region.png
   :align: center

   Duplicating a region

Delete
~~~~~~
Delete the selected objects.
You can also press
:kbd:`Delete`.

Selections
----------
Clear Selection
~~~~~~~~~~~~~~~
Clear current selection (unselect all objects).

Select All
~~~~~~~~~~
Select all objects in the current editor
(:kbd:`Control-a`).

Loop Selection
~~~~~~~~~~~~~~
Place the loop markers around the selection
(:kbd:`Control-l`).
