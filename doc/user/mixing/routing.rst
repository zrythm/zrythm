.. SPDX-FileCopyrightText: © 2020, 2022 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
   This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

Routing
=======

Groups
------
:ref:`Audio group tracks <tracks/track-types:Audio Group Track>`
and
:ref:`MIDI group tracks <tracks/track-types:MIDI Group Track>`
can be created for grouping signals.

To route channels to these tracks, you can click the
direct out selector and choose them, similar to
how it works in the
:ref:`track properties in the track inspector <tracks/inspector-page:Track Properties>`.

.. hint:: The Master track is also a group.

Buses/FX Tracks
---------------

:ref:`Audio FX tracks <tracks/track-types:Audio FX Track>`
and
:ref:`MIDI FX tracks <tracks/track-types:MIDI FX Track>`
can be created for sending additional signals to
them, in addition to the channel's main direct
output.

This is useful for example for applying reverb and
controlling the reverb signal separately.

You can send to these tracks/channels using the Sends
section in the track inspector explained in
:ref:`track-sends`.

.. note:: Some other DAWs use the term `bus` to
   refer to what Zrythm refers to as `group`.
   Please keep this in mind if you are coming from
   other DAWs as they are different things in Zrythm.

Sidechaining
------------
Similar to sending signals to FX tracks, you can
also send signals to plugin sidechain ports, if the
plugin has any. See :ref:`track-sends` for details.
