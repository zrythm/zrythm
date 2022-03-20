.. This is part of the Zrythm Manual.
   Copyright (C) 2020, 2022 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _automation-editor:

Automation Editor
=================
The automation editor is displayed when an automation
region is selected.

.. image:: /_static/img/automation-editor.png
   :align: center

It is very similar to the piano roll, with the exception
that the arranger contains automation points and curves
instead of MIDI notes.

Automation Arranger
-------------------
The automation arranger refers to the arranger section of
the automation editor.

.. image:: /_static/img/automation-arranger.png
   :align: center

The automation arranger contains automation points drawn
as circles and curves connecting them. During playback,
Zrythm will interpolate the value of the control being
automated based on the curve.

Editing inside the automation arranger generally follows
:ref:`edit-tools` and :ref:`common-operations`, with the
added feature that curves can be dragged up or down to
change their `curviness`.

.. _automation-curves:

Curves
------
Right-clicking on a curve allows you to change the curve
algorithm used. The following curve algorithms are available.

Exponent
  Exponential curve.
Superellipse
  Elliptical curve.
Vital
  The curve that Vital synth uses, similar to exponential
  curve.
Pulse
  Pulse wave.

Context Menu
------------
.. todo:: Write this section.
