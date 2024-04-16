.. SPDX-FileCopyrightText: © 2019-2020, 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
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

You can choose which parameter you want to automate in each automation lane
by clicking on the parameter name.
A popover will appear that allows you to find an automatable parameter to automate.

.. figure:: /_static/img/automatable-selector.png
   :align: center

   Selecting an automatable control

Playback Mode
-------------

Automation can be enabled or disabled using the
:guilabel:`On` and :guilabel:`Off` toggles in the track lane.

Touch/latch modes can also be used to record automation live
(see the
:ref:`playback-and-recording/recording:Recording Automation`
section).

Editing Automation
------------------
Editing automation involves creating automation
regions and then creating automation events inside
the :ref:`automation editor <editing/automation-editor:Automation Editor>`.

Showing/Hiding Lanes
--------------------

New lanes can be shown by clicking on the :guilabel:`+` button.
This will display a new automation lane assigned to the next available
automatable parameter.

.. tip:: Previously shown automation lanes will be given priority.

Existing lanes can be hidden using the :guilabel:`-` button.

.. seealso:: :ref:`tracks/track-operations:Automation Section`

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
