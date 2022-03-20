.. This is part of the Zrythm Manual.
   Copyright (C) 2020, 2022 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _automation-editor:

Automation Editor
=================
The automation editor is displayed when an automation
region is selected.

.. figure:: /_static/img/automation-editor.png
   :align: center

   Automation editor

Automation Arranger
-------------------
The automation arranger refers to the arranger
section of the automation editor.

.. figure:: /_static/img/automation-arranger.png
   :align: center

   Automation arranger

The automation arranger contains automation points
drawn as circles and curves connecting them. During
playback, Zrythm will interpolate the value of the
control being automated based on the curve.

Editing inside the automation arranger generally
follows :ref:`edit-tools` and
:ref:`common-operations`, with the
added feature that curves can be dragged up or down
to change their `curviness`.

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

.. todo:: Create table with illustrations.

Automation Functions
--------------------

Automation functions are logic that can be applied
to transform the selected automation points/curves.

The following functions are available.

Flip
  Reverse/flip the selected automation horizontally
  or vertically.
