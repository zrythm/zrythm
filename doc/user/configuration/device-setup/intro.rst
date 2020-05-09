.. This is part of the Zrythm Manual.
   Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Device Setup
============

Connecting MIDI and Audio Devices
---------------------------------

Depending on the selected backend
(covered in :ref:`preferences-engine`), the
configuration differs. Zrythm will auto-scan and allow you
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

You can select these in :ref:`midi_devices` in the welcome
dialog as well as in :ref:`preferences`.

JACK
----

When using the JACK audio and MIDI backend
Zrythm exposes ports to JACK, so devices can
be attached there using a tool like
`Qjackctl <https://qjackctl.sourceforge.io/>`_.

.. image:: /_static/img/midi-devices.png
   :align: center

.. note:: For most users, everything can be accomplished from
  within Zrythm.

.. note:: Depending on your setup, MIDI devices might need
  to be bridged to JACK using ``a2jmidid``.
