.. SPDX-FileCopyrightText: © 2020, 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
.. SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. _automation-editor:

Automation Editor
=================
The automation editor is displayed when an automation
region is selected.

.. figure:: /_static/img/automation-editor.png
   :align: center

   Automation editor

The automation arranger contains automation points
drawn as circles and curves connecting them. During
playback, Zrythm will interpolate the value of the
control being automated based on the curve.

Curves can be dragged up or down to change their `curviness`.

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

