.. Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>

   This file is part of Zrythm

   Zrythm is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   Zrythm is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU General Affero Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.

Ports
=====

Plugins expose Ports that are used internally
to route MIDI and audio signals to/from and
externally for automation.

A Port can only be an Input Port or an Output
Port and can have one of the following types.

Audio
  Ports of this type receive or send raw
  audio signals. Usually, Effect Plugins will
  have at least two of these as inputs for
  Left and Right, and at least two as outputs.
Event
  Event Ports are mainly used for routing MIDI
  signals. Instrument Plugins will have at
  least one Event Port.
Control
  Control Ports are Plugin parameters that
  are usually shown inside the Plugin's UI.
  These can be automated in automation lanes.
CV
  CV Ports are continuous signals that can be
  fed into or emitted from Plugins, and are
  mainly used by the Modulators. Each Modulator
  will have at least one CV output Port which
  can be routed to Plugin Control Ports for
  automation.

Usually, only Ports of the same type can be
connected, with the exception of CV ports.
CV output Ports may be routed to both CV
input Ports and Control input Ports.

Output Ports may only be routed to Input Ports
and vice versa.

.. note::
  Channels also have their own Ports, for
  example for the Fader, Pan, and Enabled
  (On/Off).
