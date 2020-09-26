.. This is part of the Zrythm Manual.
   Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. sectionauthor:: Alexandros Theodotou <alex@zrythm.org>

.. _mixing-overview:

Mixing Overview
===============
Mixing refers to any actions made inside the mixer,
such as routing signals around, controlling the
fader level of various channels and setting their
stereo balance.

.. image:: /_static/img/mixer.png
   :align: center

.. _mixer:

Mixer
-----
The mixer contains all of the channels in the
project corresponding to visible tracks.

The master channel,
a special channel used to route audio to the output audio
device, is pinned on the right side of the mixer.

.. image:: /_static/img/master-channel.png
   :align: center

The rest of the mixer shows the rest of the channels, along
with some extra space used for drag-and-dropping
plugins or files.

Most of the activities that can be performed in the mixer
are also available in the track inspector described in
:ref:`track-inspector`.

.. _channels:

Channels
--------
A channel is part of a track, if the track type has a
channel.

.. image:: /_static/img/master-channel.png
   :align: center

Channels contain a fader, controls such as
mute and solo, a meter, a direct out selector and
a strip of insert plugins. These are covered in
:ref:`track-inspector`.

Changing the Track Name
~~~~~~~~~~~~~~~~~~~~~~~
The name of the track the channel corresponds to can
be changed by double clicking the name at the top of
the channel.

.. todo:: Add pic.
