.. This is part of the Zrythm Manual.
   Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _automation:

Automation
==========

Automation Lanes
----------------
Tracks which have automatable controls, such
as Fader, Pan and parameters of Plugins they
contain will have an option to show their
Automation Lanes.

You can choose which parameter you want to
automate in each Automation Lane.

.. image:: /_static/img/automatable-selector.png
   :align: center

Editing Automation
------------------
Editing automation consists of creating automation regions
and then creating automation events inside the automation
editor. See the :ref:`editing` chapter for details.

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
