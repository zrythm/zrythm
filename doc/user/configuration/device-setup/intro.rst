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

Device Setup
============

Connecting MIDI and Audio Devices
---------------------------------

On Linux machines, Zrythm works with both ALSA and JACK as
available backends. Depending on the backend selected, the
configuration differs.

Auto-Connecting Devices
-----------------------

Zrythm has an option to select devices to
auto-connect on launch. TODO explain it.

JACK
----

When using the JACK audio and MIDI backend,
Zrythm exposes ports to JACK, so devices can
be attached there using a tool like Catia.
Note that for MIDI, devices might need to be
bridged to JACK using ``a2jmidid``.

An example configuration looks like this (in Catia inside Cadence)

.. image:: /_static/img/midi_devices.png

ALSA
----

A tool like Catia can be used to connect
MIDI devices to Zrythm.
