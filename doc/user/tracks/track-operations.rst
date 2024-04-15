.. SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. _track-operations:

Track Operations
================

Track Selection
---------------

Tracks are selected by clicking on them, either in the tracklist or in the mixer.
Multiple tracks can be selected by pressing and holding :kbd:`Control` while clicking them.

.. seealso:: There is also :ref:`an option to auto-select tracks when a track object is selected in the timeline <tracks/tracklist:Tracklist Settings>`.

Moving/Copying Tracks
---------------------
Tracks can be moved by clicking on one of the selected tracks and dragging and dropping them to another location.
The drop locations will be highlighted as you move the Track.

Holding down :kbd:`Control` while dropping will cause the tracks to be
cloned (copied) instead of moved.

.. figure:: /_static/img/moving-tracks.png
   :align: center

   Moving a track above the "Audio FX Track"

Changing Track Icon/Color/Name
------------------------------

Track Icon
~~~~~~~~~~

The icon can be changed by clicking on the icon in the tracklist.

Track Color
~~~~~~~~~~~

The track color can be changed by clicking on the color box on the left side of the track view, or via the :guilabel:`Change Color` context menu option.

Track Name
~~~~~~~~~~

The track name can be changed by double-clicking on it in the track view, in the track inspector, or in the channel.
It can also be changed via the :guilabel:`Rename` context menu option.

.. Lock
    Prevent any edits on the track while locked.

  Freeze
    Bounce the track to an audio file internally and
    prevent edits while frozen. This is intended to
    reduce :term:`DSP` load on DSP-hungry tracks.

Context Menu Options
--------------------

Each track has a context menu with options that vary depending on its type. The context menu can be activated by right-clicking on one or more tracks.

.. figure:: /_static/img/track-context-menu.png
   :align: center

   Track context menu

Edit Section
~~~~~~~~~~~~

Delete
  Deletes the selected tracks.
Duplicate
  Duplicates the selected tracks.
Hide
  Hides the selected tracks.

  .. seealso:: See :ref:`tracks/tracklist:Track Visibility and Filtering` for more information about track visibility.

Pin/Unpin
  Pins the selected tracks.

  .. seealso:: See :ref:`tracks/tracklist:Tracklist` for details.

Change Color
  Changes the color of the selected track.
Rename
  Changes the name of the selected track.

Selection Section
~~~~~~~~~~~~~~~~~

Append Track Objects to Selection
  Appends all track objects to the currently selected timeline objects.

  .. hint:: This is useful for performing operations on all track objects.

Bounce Section
~~~~~~~~~~~~~~

Quick Bounce
  Bounce with last known settings.
Bounce...
  Show the Bounce dialog with bounce settings.

Channel Section
~~~~~~~~~~~~~~~

[Fader Controls]
  See :ref:`tracks/inspector-page:Fader`.
Direct Output
  Allows changing the direct output of the selected tracks to either an existing track or to a newly-created group track.
Disable
  Disables the selected tracks. Disabled tracks will not be processed.

Piano Roll Section
~~~~~~~~~~~~~~~~~~

Track MIDI Channel
  The MIDI channel that MIDI events from the piano roll will be assigned to.

Automation Section
~~~~~~~~~~~~~~~~~~

Show Used Lanes
  Show all lanes that contain automation.
Hide Unused Lanes
  Hide all lanes that do not contain automation.
