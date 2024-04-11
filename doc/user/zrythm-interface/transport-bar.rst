.. SPDX-FileCopyrightText: © 2019, 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. _transport-bar:

Transport Bar
=============

The transport bar contains information about the audio engine,
:ref:`the BPM and time signature <playback-and-recording/bpm-and-time-signatures:BPM and Time Signatures>`,
:ref:`transport controls <playback-and-recording/transport-controls:Transport Controls>`.
and various indicators.

.. figure:: /_static/img/transport-bar.png
   :align: center

   Transport bar

Backend Information
-------------------
Information about the currently selected backend and
options is visible in the bottom left corner.

.. figure:: /_static/img/backend-info.png
   :align: center

   Backend info

If the backend supports it, the buffer size can be
changed live by clicking the :guilabel:`change`
button.

If the backend supports it, the engine can be
temporarily disabled by clicking the
:guilabel:`Disable` button.

Indicators
----------

MIDI In
  Shows the :term:`MIDI` activity of auto-connected MIDI devices.
Oscilloscope
  Shows the real-time audio waveform from the master output.
Spectrum Analyzer
  Shows the real-time audio spectrum of the master output.
CPU/DSP Usage
  Displays the current CPU % usage at the
  top and the current DSP % usage at the bottom. If
  the DSP % exceeds 100 (all bars filled), audio will
  start clipping.
