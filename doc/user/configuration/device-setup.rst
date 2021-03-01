.. This is part of the Zrythm Manual.
   Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Device Setup
============

Connecting MIDI and Audio Devices
---------------------------------

Zrythm will auto-connect to the devices specified in
:ref:`configuration/preferences:Audio inputs`
and
:ref:`configuration/preferences:MIDI controllers`
and make these devices available in the
:ref:`track inputs <tracks/inspector-page:Inputs>`
(as below) and other places.

.. image:: /_static/img/track_inputs.png
   :align: center

JACK
----

When using the JACK audio and MIDI backend
Zrythm exposes ports to JACK, so devices can
be attached there using a tool like
`Qjackctl <https://qjackctl.sourceforge.io/>`_.

.. image:: /_static/img/midi-devices.png
   :align: center

.. tip:: Depending on your setup, MIDI devices might need
  to be bridged to JACK using :term:`a2jmidid`.

.. warning:: Zrythm will not remember connections made
   externally.
