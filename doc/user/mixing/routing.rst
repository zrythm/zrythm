.. This is part of the Zrythm Manual.
   Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Routing
=======

Groups
------
Audio and MIDI group tracks (covered in
:ref:`audio-group-track` and :ref:`midi-group-track`
respectively) can be created for grouping
signals.

To route channels to these tracks, you can click the
direct out selector and choose them.

.. image:: /_static/img/routing-to-groups.png
   :align: center

Buses/FX Tracks
---------------
Audio and MIDI FX tracks (covered in :ref:`audio-bus-track`
and :ref:`midi-bus-track` respectively) can be created
for sending additional signals to them, in addition to
the channel's main direct output.

This is useful for example for applying reverb and
controlling the reverb signal separately.

You can send to these tracks/channels using the Sends
section in the track inspector explained in
:ref:`track-sends`.

.. note:: Some other DAWs use the term `bus` to refer to what
  Zrythm refers to as `group`. Please keep this in mind if you
  are coming from other DAWs as they are different things in
  Zrythm.

Sidechaining
------------
Similar to sending signals to FX tracks, you can also send
signals to plugin sidechain ports, if the plugin has
any. See :ref:`track-sends` for details.
