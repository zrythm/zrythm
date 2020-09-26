.. This is part of the Zrythm Manual.
   Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _timeline-arranger:

Timeline Arranger
=================

The timeline arranger is the main area where the
song is composed. It consists of a collection of
events, such as regions, positioned against time.
Some events will open separate windows for further
editing when clicked.

.. image:: /_static/img/timeline-arranger.png
   :align: center

The timeline arranger is split into a top arranger
that remains fixed on top and a bottom arranger
below it. This way you can pin tracks you want to
always be visible at the top.

.. todo:: split the following to sub-pages

Arranger Objects
----------------
The following arranger objects can exist inside
the timeline.

.. _regions:

Regions
~~~~~~~
Regions (or :term:`clips <clip>`) are containers
for events or data (such as MIDI notes and audio
clips - see below) that can be edited in an
:ref:`editor <editors>`. Regions can be repeated,
like below.

.. image:: /_static/img/region.png
   :align: center

The following types of regions exist.

Audio Region
++++++++++++
Contains audio clips.

.. todo:: Expand + add pic.

Fades
^^^^^
.. todo:: Explain how fades work.

.. todo:: Decide what to do about cross-fades
   (just use fades?).

MIDI Region
+++++++++++
Contains MIDI notes.

.. todo:: Expand + add pic.

Automation Region
+++++++++++++++++
Contains automation events.

.. todo:: Expand + add pic.

Chord region
++++++++++++
Contains sequences of chords.

.. todo:: Expand + add pic.

Markers
~~~~~~~
Markers are used to mark the start of a logical
section inside the song, such as `Chorus` or
`Intro`.

.. image:: /_static/img/marker.png
   :align: center

There are two special markers that signify the
start and end of the song that are used for
exporting the song and cannot be deleted.

Scales
~~~~~~
Scales are used in the Chord track to indicate the
start of a section using a specific musical scale.

.. image:: /_static/img/scale-object.png
   :align: center

Editing Objects
---------------
The following operations apply to all editable
objects.

Creating
~~~~~~~~
In select (default) mode, all objects are created by
double clicking inside the arranger and dragging,
then releasing when you are satisfied with the
position/size.

For creating objects in other modes, see
:ref:`edit-modes`.

Moving
~~~~~~
Objects are moved by clicking and dragging them
around. You can move regions to other tracks if
the track types are compatible.

Copy-Moving
~~~~~~~~~~~
Holding down :kbd:`Ctrl` while moving objects will
allow you to copy-and-move the objects to the new
location.

Editing Regions
---------------
The following operations apply to regions.

Looping
~~~~~~~
Regions can be repeated, and hence they have
:ref:`editable loop points and a clip start position <editing/clip-editors/ruler:Loop Points>`
in the
:doc:`Editor Ruler <../../editing/clip-editors/ruler>`
to modify the looping (repeating) behavior.

Regions can also be looped inside the timeline,
by moving the cursor to the bottom-left or
bottom-right edge of the region, then clicking
and dragging.

.. figure:: /_static/img/looping-regions.png
   :align: center

   Looping (loop-resizing) a region

.. note:: If the region is already repeated, it
   cannot be resized anymore until its loop points
   match exactly the region's start and end points.

Link-Moving
~~~~~~~~~~~
Linked regions can be created by holding down
:kbd:`Alt` while moving.

.. todo:: Add pic to show this.

You can verify that a link exists on a region by
the link icon that shows in the top right.

.. todo:: Add pic to show this.

Renaming
~~~~~~~~
Regions can be renamed by clicking on their name.

.. todo:: Illustrate.

Other editing capabilities were explained
previously in :ref:`edit-modes`.
