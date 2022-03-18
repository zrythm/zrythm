.. This is part of the Zrythm Manual.
   Copyright (C) 2019-2020, 2022 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _automation:

Automation
==========

Automation Tracks
-----------------
Automation tracks (or automation *lanes*) are used to
create automation
events for various automatable parameters/controls,
such as fader volume, fader balance, plugin controls
and MIDI controls, if the track type supports it.

To show the automation lanes for a track, click the
cusp icon.

.. figure:: /_static/img/automation-track.png
   :align: center

   Automation track for Fader Volume

You can choose which parameter you want to
automate in each automation lane.

.. figure:: /_static/img/automatable-selector.png
   :align: center

   Selecting an automatable control

Editing Automation
------------------
Editing automation involves creating automation
regions and then creating automation events inside
the automation editor. See
:ref:`editing/clip-editors/automation-editor:Automation Editor`
for details.

Behavior
--------
The control corresponding to each automation lane
will use the value from the automation lane if
there are automation events present at the current
playhead position.

If there is no automation at the playhead, the
control will use the last known value, either from
the automation lane or from a manual change.

This behavior also applies when rendering (exporting)
the project.
