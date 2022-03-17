.. This is part of the Zrythm Manual.
   Copyright (C) 2019, 2022 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _transport-bar:

Transport Bar
=============

The transport bar contains information about the
audio engine, transport controls and CPU/DSP usage.

.. figure:: /_static/img/transport-bar.png
   :align: center

   Transport bar

BPM and Time Signature
----------------------
BPM and Time Signature are covered in
:ref:`playback-and-recording/bpm-and-time-signatures:BPM and Time Signatures`.

Transport Controls
------------------
Transport controls are covered in
:ref:`playback-and-recording/transport-controls:Transport Controls`.

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

CPU/DSP Usage
-------------

.. figure:: /_static/img/cpu-dsp-usage.png
   :align: center

   CPU/DSP usage

This widget displays the current CPU % usage at the
top and the current DSP % usage at the bottom. If
the DSP % exceeds 100 (all bars filled), audio will
start clipping.
