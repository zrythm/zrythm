.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

First Run Wizard
================

When you first run Zrythm, it will display a wizard that lets
you configure the basic settings that Zrythm will use. These
include MIDI devices, the default Zrythm path, interface
language and audio/MIDI backends.

Language Selection
------------------

.. image:: /_static/img/first-run-language.png
   :align: center

Zrythm lets you choose the language of the interface. The
interface is already translated in multiple languages, so
choose the language you are most comfortable in.

.. note:: You must have a locale enabled for the language
  you want to use.

Path
----

.. image:: /_static/img/first-run-path.png
   :align: center

This is the path where Zrythm will save projects,
temporary files, exported audio, etc. The default is
"zrythm" under
`XDG_DATA_HOME <https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html>`_ on freedesktop-compliant
systems.

Audio/MIDI Backends
-------------------

.. image:: /_static/img/first-run-backends.png
   :align: center

Zrythm supports multiple audio and MIDI backend engines.
JACK is the recommended one for both, but it takes some time
to set up if this is your first time using it. If you choose
to use JACK, JACK MIDI and the JACK audio backend must be
chosen together.

If JACK is not available on your system,
we recommend RtAudio and RtMidi.

Click :zbutton:`Test` to try connecting to the backend to
see if it is functional.

.. note:: If you choose to use JACK, JACK MIDI and the JACK
   audio backend must be chosen together.

.. _midi_devices:

MIDI Devices
------------

.. image:: /_static/img/first-run-midi-devices.png
   :align: center

These are the discovered devices that will be auto-connected
and ready to use every time you run Zrythm. Click "Rescan"
to scan for devices again.

.. tip:: All of the settings mentioned here are also
   available in the :ref:`preferences`, so don't worry
   if you selected the wrong settings.
