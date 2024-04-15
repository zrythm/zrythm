.. SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

Tracklist
=========

Overview
--------

The tracklist contains all of the Tracks in the project.
It is split into the top part showing pinned tracks (in the case below the
Chord and Marker tracks), and the bottom part showing unpinned tracks.

.. figure:: /_static/img/tracklist.png
   :align: center

   The project tracklist

At the top of the tracklist, the number of visible tracks
over the number of total tracks is shown (in the screenshot above, this means 7 visible tracks out of 9 total tracks).

Pinning Tracks
~~~~~~~~~~~~~~
You can pin tracks to make them stick to the top, regarldess how far you
scroll down in the tracklist.
This is useful to do with chords and markers so you can always see where
you are musically/structurally in your project.
Tracks are pinned by selecting :guilabel:`Pin/Unpin Track` in their context menu.

.. hint:: Zrythm pins the Chord and Marker tracks by default.

Track Visibility and Filtering
------------------------------

The track visibility button in the top of the tracklist allows changing the
visible state of tracks. It is also possible to filter the tracks currently shown in the tracklist.

.. figure:: /_static/img/track-visibility.png
   :align: center

   Track visibility and filtering options

Tracks can be filtered by name, track type and active status. There is also a visibility table for toggling the visible state of individual tracks.

.. tip::
   Tracks can also be quickly hidden with the :guilabel:`Hide` option in their context menu.

Tracklist Settings
------------------

These are global behavior settings that apply to all tracks.

.. figure:: /_static/img/tracklist-settings.png
   :align: center

   Tracklist settings

Auto-arm for Recording
  Arm tracks for recording when clicked/selected.
Auto-select
  Select tracks when their regions are clicked/selected.
