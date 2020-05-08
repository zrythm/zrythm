.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _export-audio-and-midi:

Audio & MIDI
============

The Export dialog below is used to export the project
or part of the project into audio or MIDI files.

.. image:: /_static/img/export_dialog.png
   :align: center

Fields
------

Artist and Genre
~~~~~~~~~~~~~~~~

These will be included as metadata to the exported
file if the format supports it. The title used will
be the project title.

Format
~~~~~~

The format to export to. Currently, the following
formats are supported

* FLAC - .FLAC
* OGG (Vorbis) - .ogg
* WAV - .wav
* MP3 - .mp3
* MIDI - .mid

Dither
~~~~~~

TODO

Bit Depth
~~~~~~~~~

This is the bit depth that will be used when exporting
audio. The higher the bit depth the larger the file
will be, but it will have better quality.

Time Range
~~~~~~~~~~

The time range to export. You can choose to export the
whole song (defined by the start/end markers), the
current loop range or a custom time range.

Filename Pattern
~~~~~~~~~~~~~~~~

The pattern to use as the name of the file.

Open Exported Directory
-----------------------
Once export is completed, a dialog will appear with an
option to open the directory the file was saved in
using your default file browser program.

.. image:: /_static/img/export_finished.png
   :align: center
