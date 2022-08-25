.. SPDX-FileCopyrightText: © 2019-2020, 2022 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

Overview
========

Tracks are the main building blocks of projects.
Tracks appear in the tracklist and contain various
events such as :ref:`regions <regions>`.

.. figure:: /_static/img/track.png
   :align: center

   Track in tracklist

Most types of tracks have a corresponding channel
that appears in the :ref:`Mixer <mixer>`. See
:ref:`channels` for more info.

.. figure:: /_static/img/channel.png
   :align: center

   Channel in mixer

There are various kinds of tracks suited for
different purposes, explained in the following
sections. Some tracks are special, like the
:ref:`Chord Track <tracks/track-types:Chord Track>`
and the
:ref:`Marker Track <tracks/track-types:Marker Track>`,
which contain chords and markers respectively.

.. important:: In Zrythm, group tracks are used for
  grouping signals (direct routing), FX/bus tracks
  are used for effects/sends, and folder tracks
  (coming soon) are  used for
  putting tracks under the same folder and
  performing common operations.

  Moreover, Zrythm uses Instrument tracks for
  instrument plugins, such as synthesizers, and
  MIDI tracks for MIDI plugins, such as a
  MIDI arpegiator.

  This is different from what many
  other :term:`DAWs <DAW>` do, so please keep
  this in mind.

Track Interface
---------------

.. image:: /_static/img/track-interface.png
   :align: center

Each track has a color, an icon (corresponding to its type)
a name and various buttons. Tracks that can have
lanes, like instrument tracks, will also have an option to
display each lane as above. Tracks that can have automation
will have an option to display automation tracks as above.

If the track produces output, it will have a meter on its
right-hand side showing the current level.

Track Icon
~~~~~~~~~~
The icon can be changed by clicking on the icon in
the tracklist.

.. todo:: Add pic and explain how to add custom
   icons.

Context Menu
------------

Each tracks has a context menu with additional options
depending on its type.

.. image:: /_static/img/track-context-menu-duplicate-track.png
   :align: center

The section :ref:`track-operations` explains the
various track operations available.
