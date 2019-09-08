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

Zrythm lets you choose the language of the interface. The
interface is already translated in multiple languages, so
choose the language you are most comfortable in.

.. note:: You must have a locale for the language you want to use enabled.

	This is usually not a problem since you are probably already
	using the correct locale for your language. In case a locale
	cannot be found, you will see this message telling you the
	steps to enable it.

Path
----

.. image:: /_static/img/first-run-path.png

This is the path where Zrythm will use to save projects,
temporary files, exported audio, etc. The default is
"zrythm" in the user's directory.

Audio/MIDI Backends
-------------------

.. image:: /_static/img/first-run-backends.png

Zrythm supports multiple audio and MIDI backend engines.
JACK is the recommended one for both, but it takes some time
to set up if this is your first time using it. If you don't
want to use JACK for some reason you can select other backends
such as ALSA.

Click :zbutton:`Test` to try connecting to the backend to see if it is
functional.

.. note:: JACK MIDI requires a JACK server to be running,
	which means you probably want to use it with the JACK audio
	backend.

MIDI Devices
------------

.. image:: /_static/img/first-run-midi-devices.png

These are the discovered devices that will be auto-connected
and ready to use every time you run Zrythm. Click "Rescan"
to scan for devices again.

.. tip:: All of the settings mentioned here are also available in the
	preferences (Ctr+Shift+P or File->Preferences), so don't worry
	if you selected the wrong settings.
