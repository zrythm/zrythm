.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Preferences
===========

Zrythm has a Preferences dialog containing all
of the global settings that can be accessed by
clicking the gear icon or by ``Ctrl+Shift+P``.

.. image:: /_static/img/preferences.png
   :align: center

The Preferences dialog is split into the
following sections, which are explained below:

- General
- Audio
- GUI
- Plugins
- Projects

General
-------

Audio Backend
  The backend to use for the audio engine.

MIDI Backend
  The MIDI backend to use for the audio engine.

MIDI Controllers
  MIDI devices to auto-connect to when Zrythm starts.

Pan Algorithm
  The panning algorithm to use. See the graph below
  for the different curves as you move the pan
  from left to right. We recommend leaving it as the
  default (Sine).
  See https://www.cs.cmu.edu/~music/icm-online/readings/panlaws/index.html
  for more information.

  .. figure:: /_static/img/pan_algorithms.png
     :figwidth: image
     :align: center

     Pan algorithms (:blue:`sine`,
     :red:`square root`, :green:`linear`).

Pan Law
  This is how much to attennuate the signal when
  the pan is in the center. Without this, the signal
  would be louder when pan is in the center and more
  silent on the sides, which you likely want to
  avoid. We recommend leaving this to -3dB. See
  https://en.wikipedia.org/wiki/Pan_law for more
  details.

Zrythm Path
  The path to save projects, temporary files, and other
  non-project specific files.

Plugins
-------

Always open plugin UIs
  Always show the plugin UI when instantiating plugins.

Keep plugin UIs on top
  Whether to always keep plugin UIs above other Zrythm windows or not.

GUI
---

Language
  The language that the Zrythm interface uses.

Projects
--------

Autosave Interval
  The amount of time to wait before auto-saving a backup of the current
  project, in minutes. Setting this to 0 will turn it off.

.. note:: Changing some of these settings requires a restart of Zrythm.
