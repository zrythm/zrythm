.. This is part of the Zrythm Manual.
   Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _ports:

Ports
=====

Before getting into connections, it is necessary
to understand some basic information about ports.

Ports are the main building blocks used to
route signals throughout Zrythm. A port
does not process anything on its own, and is
usually part of a `processor`, like a plugin
or track.

For example, an instrument track has a `MIDI input`
port it uses to read MIDI data and 2
`Audio output` ports where it copies the
stereo signal after being processed.

Port Directions
---------------
Input
  The port receives signals.
Output
  The port sends signals.

Port Types
----------
Audio
  Ports of this type receive or send raw
  audio signals.
Event
  Event ports are mainly used for routing MIDI
  signals.
Control
  Control ports are user-editable parameters that
  can also be automated in automation lanes.
CV
  CV ports handle continuous signals and can be
  used to modulate control ports.

Port Connections
----------------
Ports can be connected with each other, as long
as they are of the same type and opposite direction,
with the exception of
CV ports which may be routed to both CV
ports and control ports.

When ports are connected, the signal from the
source port is added to the destination port
during processing.
