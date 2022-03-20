.. This is part of the Zrythm Manual.
   Copyright (C) 2020, 2022 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Toolbar
=======

The timeline has its own toolbar for
timeline-specific operations.

.. figure:: /_static/img/timeline-toolbar.png
   :align: center

   Timeline toolbar

It includes the following sections:

* :ref:`Snapping options <editing/snapping-grid-options:Snapping and Grid Options>`
* :ref:`Quantization options <editing/quantization:Quantization>`
* Audio region options
* Range actions
* :ref:`Playhead tracking options <editing/common-operations:Playhead Tracking>`
* Object merge options
* :ref:`Zoom options <editing/common-operations:Zooming>`
* :ref:`Event viewer options <editing/event-viewers:Event Viewers>`

Audio Region Options
--------------------

Musical Mode
~~~~~~~~~~~~
When this is on, audio regions will be automatically
time-stretched to match the new BPM, each time the BPM
changes. If this is off, changing the BPM will cause
audio regions to be repeated accordingly to match the
new BPM without being time-stretched.

Range Actions
-------------

Range actions are performed on the selected
:term:`Range`.

Insert Silence
  Inserts silence at the selected range and pushes
  all events in the range forward
Remove Range
  Deletes all events in the range and moves events
  after the range backwards

Object Merging
--------------

Merge Selections
  Merge the selected objects into a single object,
  if possible
