.. This is part of the Zrythm Manual.
   Copyright (C) 2020, 2022 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _audio-editor:

Audio Editor
============
The audio editor is displayed when an audio region is
selected.

.. figure:: /_static/img/audio-editor.png
   :align: center

   Audio editor

Audio Arranger
--------------
The audio arranger refers to the arranger section
of the audio editor.

.. figure:: /_static/img/audio-arranger.png
   :align: center

   Audio arranger

The arranger shows the following indicators and
controls.

* Waveform
* Fades
* Amplitude line
* Part selection

Changing the Amplitude
----------------------

The amplitude of the clip can be changed by clicking
and dragging the orange horizontal line up or down.

Changing Fades
--------------

Fade in/out positions can be changed by clicking and
dragging near the top where the fade in ends or where
the fade out starts.

.. hint:: The cursor will change to a resize
   left/right cursor.

Fade curviness can be changed by clicking and
dragging the body of fades upwards or downwards.

.. hint:: The cursor will change to a resize
   up cursor.

Selecting Parts
---------------

Audio parts can be selected by clicking and dragging
in the bottom half of the arranger.

.. hint:: The cursor will change to a range
   selection cursor.

Audio Functions
---------------

Audio functions are logic that can be applied
to transform the selected audio part.

The following functions are available.

Invert
  Invert the polarity of the audio.
Normalize Peak
  Normalize the audio so that its peak is 0dB.
Linear Fade In
  Fade the audio from silence to full amplitude.
Linear Fade Out
  Fade the audio from full amplitude to silence.
Nudge
  Shift the audio by 1 sample backward or forward.
Reverse
  Reverse the audio (play back backwards).
External Program
  Run an external program (such as
  `Audacity <https://www.audacityteam.org/>`_)
  to edit the audio.

  .. figure:: /_static/img/edit-audio-external-app.png
     :align: center

     Edit the selected audio in an external app

  The selected program will be run with the path to
  a temporary file as an argument. Normally, the
  application will open that file, otherwise you will
  have to open it manually.

  .. hint:: You can select and copy the path in the
     dialog.

  Once you've made your changes to the file, close
  the external app and press OK. Zrythm will then
  apply your changes to the selected audio part.

  .. important:: The length of the audio must stay
     exactly the same, otherwise this operation will
     fail.

Guile Script
  Not available yet.
