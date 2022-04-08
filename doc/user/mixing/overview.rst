.. SPDX-FileCopyrightText: © 2020, 2022 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
   This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. sectionauthor:: Alexandros Theodotou <alex@zrythm.org>

.. _mixing-overview:

Mixing Overview
===============
Mixing refers to any actions made inside the mixer,
such as routing signals around, controlling the
fader level of various channels and setting their
stereo balance.

.. figure:: /_static/img/mixer.png
   :align: center

   The mixer

.. _mixer:

Mixer
-----
The mixer contains all of the channels in the
project corresponding to visible tracks. There is
some extra space reserved  for drag-and-dropping
plugins or files.

Most of the activities that can be performed in the
mixer are also available in the
:ref:`Track inspector <tracks/inspector-page:Track Inspector>`.

.. _channels:

Channels
--------
A channel is part of a track, if the track type has a
channel.

.. figure:: /_static/img/channel.png
   :align: center

   Channel for an instrument track

Channels contain a fader, controls such as
mute and solo, a meter, a direct out selector and
a strip of insert plugins. These are covered in
:ref:`track-inspector`.

Master Channel
~~~~~~~~~~~~~~

The master channel, a special channel used to route
audio to the output audio device, is pinned on the
right side of the mixer.

.. figure:: /_static/img/master-channel.png
   :align: center

   The master channel

Changing the Track Name
~~~~~~~~~~~~~~~~~~~~~~~
The name of the track the channel corresponds to can
be changed by double clicking the name at the top of
the channel.

.. figure:: /_static/img/changing-channel-name.png
   :align: center

   Changing the track name from the channel view
