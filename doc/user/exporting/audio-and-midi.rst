.. SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
   This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. _export-audio-and-midi:

Audio & MIDI
============

The Export dialog below is used to export the project
or part of the project into audio or MIDI files.
It can be accessed by clicking
:ref:`zrythm-interface/main-toolbar:Export`.

.. figure:: /_static/img/export-dialog.png
   :align: center

   Export dialog

Fields
------

Artist and Genre
~~~~~~~~~~~~~~~~
These will be included as metadata to the exported
file if the format supports it. The title used will
be the project title.

Format
~~~~~~
The format to export to. The formats mentioned in
:ref:`Supported Formats <plugins-files/audio-midi-files/overview:Supported Formats>`
are available, with the exception of MP3.

Dither
~~~~~~

.. todo:: Implement.

Bit Depth
~~~~~~~~~
This is the bit depth that will be used when
exporting audio. The higher the bit depth the
larger the file will be, but it will have better
quality.

Time Range
~~~~~~~~~~
The time range to export. You can choose to export
the whole song (defined by the start/end markers),
the current loop range or a custom time range.

Filename Pattern
~~~~~~~~~~~~~~~~
The pattern to use as the name of the file.

Mixdown
-------
When exporting the mixdown, sound from all selected
tracks will be included in the resulting file. This
is the option to use when exporting your song.

.. figure:: /_static/img/export-dialog-mixdown.png
   :align: center

   Export options for mixdown

.. figure:: /_static/img/export-mixdown.png
   :align: center

   Exported file

Stems
-----
Exporting stems means that each selected track
will be exported in its own file. This is useful
when you want to share the components of your song
separately. For example, you can
:term:`bounce <Bounce>` the drums and the bass
as separate audio files.

.. figure:: /_static/img/export-dialog-stems.png
   :align: center

   Export options for stems

.. figure:: /_static/img/export-stems.png
   :align: center

   Exported files
