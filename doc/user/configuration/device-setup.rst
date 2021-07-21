.. This is part of the Zrythm Manual.
   Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Devices
=======

Device Setup
++++++++++++

Connecting MIDI and Audio Devices
---------------------------------

Zrythm will auto-connect to the devices specified in
:ref:`configuration/preferences:Engine`
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

.. tip:: ALSA MIDI devices might need to be bridged
   to JACK using :term:`a2jmidid`.

.. warning:: Zrythm will not remember connections
   made externally.

MIDI Bindings
+++++++++++++

Creating Bindings
-----------------

MIDI device controls can be mapped to controls
inside Zrythm (on controls that support it). After
enabling a device in the
:ref:`preferences <configuration/preferences:Engine>`,
a mapping can be created by right-clicking on
eligible controls and selecting
:guilabel:`MIDI learn`.

.. figure:: /_static/img/midi-learn-sends.png
   :align: center

   MIDI learn on channel sends

A window will show up asking you to press a key or
move a knob on your MIDI device.

.. figure:: /_static/img/midi-learn-popup.png
   :align: center

   MIDI learn window

When the mapping is confirmed, the selected control
can be controlled using the MIDI device.

Managing Bindings
-----------------

Device
mappings (bindings) can be found under the
:guilabel:`Bindings` tab in the main panel.

.. figure:: /_static/img/midi-bindings.png
   :align: center

   MIDI CC bindings

Bindings can be deleted by right-clicking on a
row and selecting :guilabel:`Delete`.

.. figure:: /_static/img/midi-bindings-delete.png
   :align: center

   Deleting a binding
