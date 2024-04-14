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

   A track in the tracklist

Most types of tracks have a corresponding channel
that appears in the :ref:`Mixer <mixer>`. See
:ref:`channels` for more info.

.. figure:: /_static/img/channel.png
   :align: center

   A channel in the mixer

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

  This may be different from what most other :term:`DAWs <DAW>` do.

Track Interface
---------------

.. figure:: /_static/img/track-interface.png
   :align: center

   Track interface

Each track has a color, an icon (corresponding to its type)
a name and various buttons. Tracks that can have
lanes, like instrument tracks, will also have an option to
display each lane as above. Tracks that can have automation
will have an option to display automation tracks as above.

If the track produces output, it will have a meter on its
right-hand side showing the current level.
