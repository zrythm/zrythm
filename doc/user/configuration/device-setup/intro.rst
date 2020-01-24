.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Device Setup
============

Connecting MIDI and Audio Devices
---------------------------------

On Linux-based platforms, Zrythm works with both ALSA
and JACK as
available backends. Depending on the selected backend, the
configuration differs.

Zrythm will auto-scan and allow you
to connect to input devices for recording through the
Track Inspector, as below, and in most cases you don't
need to use any external tools or auto-connect mechanism.

.. image:: /_static/img/track_inputs.png
   :align: center

Auto-Connecting Devices
-----------------------

For other types of devices that are not linked to specific
tracks, such as devices that send global
MIDI messages and devices that control the transport,
Zrythm has an option to select these devices to
auto-connect to on launch.

This is a TODO feature

JACK
----

When using the JACK audio and MIDI backend,
Zrythm exposes ports to JACK, so devices can
be attached there using a tool like Catia.
Note that for MIDI, devices might need to be
bridged to JACK using ``a2jmidid``.

An example configuration looks like this (in Catia inside Cadence)

.. image:: /_static/img/midi_devices.png
   :align: center

ALSA
----

A tool like Catia can be used to connect
MIDI devices to Zrythm.
