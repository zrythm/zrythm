.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

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
