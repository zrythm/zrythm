.. SPDX-FileCopyrightText: © 2019-2020, 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
   This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

Common Operations
=================

Editing refers to any work done in the timeline arranger
and the editors.
The following operations are common to most editors.

Undo/Redo
---------
Most actions in Zrythm are undoable, which means
that you can go back to the state before that
action was made, or you can come back to a state
that you've undone from.

For example, if you create a MIDI note and press
undo, the MIDI note will be deleted (Zrythm goes
back into the state where the MIDI note did not
exist). Pressing the :guilabel:`Redo` button in the :ref:`main toolbar <zrythm-interface/main-toolbar:Main Toolbar>` will
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

.. warning:: If you undo and then perform a new action, the :guilabel:`Redo` history will be cleared.

Object Operations
-----------------

The following operations are generally available in each editor toolbar and/or in context menus.
Some operations (like panning) are only possible via mouse and keyboard input.
We recommend learning and using the shortcuts and gestures for better productivity.

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
Delete the selected objects. The shortcut for deleting is
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

Zooming
-------

The timeline arranger and each arranger in the editor
include horizontal (and sometimes vertical) zooming functionality.

Zooming in/out horizontally can be conveniently performed
by moving the cursor to the desired location and
holding down :kbd:`Control` while scrolling up with
a mouse wheel to zoom in or scrolling down to zoom out.

Holding down :kbd:`Control-Shift` while scrolling will zoom in/out
vertically instead (in editors that support vertical zooming).

Panning
-------

Pressing and holding :kbd:`Alt`, then clicking and dragging in an editor (or ruler)
allows panning (moving) the view based on the location initially clicked.
Panning is also possible by clicking and dragging using the middle button on a mouse/pointer device (without using the :kbd:`Alt` button).

Zrythm also supports two-finger scroll on mousepads (like on laptops) to pan views.

Common Key Modifiers and Shortcuts
----------------------------------

Generally, :kbd:`Shift` is used to bypass snapping, and :kbd:`Escape` will cancel a currently in-progress action.
