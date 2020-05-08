.. This is part of the Zrythm Manual.
   Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _timeline-arranger:

Timeline Arranger
=================

The timeline arranger is the main area where the song is
composed. It consists of a collection of events, such as
regions, positioned against time. Some events will open
separate windows for further editing when clicked.

.. image:: /_static/img/timeline-arranger.png
   :align: center

The timeline arranger is split into a top arranger that
remains fixed on top and a bottom arranger below it. This
way you can pin tracks you want to always be visible at
the top.

Arranger Objects
----------------
The following arranger objects can exist inside the timeline.

Regions
~~~~~~~
Regions are containers for events or data (such as audio
clips).

.. image:: /_static/img/region.png
   :align: center

The following types of regions exist.

Audio region
  Contains audio clips.
MIDI region
  Contains MIDI notes.
Automation region
  Contains automation events.
Chord region
  Contains sequences of chords.

Markers
~~~~~~~
Markers are used to mark the start of a logical section
inside the song, such as `Chorus` or `Intro`.

.. image:: /_static/img/marker.png
   :align: center

There are two special markers that signify the start and
end of the song that are used for exporting the song and
cannot be deleted.

Scales
~~~~~~
Scales are used in the Chord track to indicate the start
of a section using a specific musical scale.

.. image:: /_static/img/scale-object.png
   :align: center

Creating Objects
----------------
In select (default) mode, all objects are created by
double clicking inside the arranger and dragging, then
releasing when you are satisfied with the position/size.

For creating objects in other modes, see :ref:`edit-modes`.

Moving Objects
--------------
Objects are moved by clicking and dragging them around.
You can move regions to other tracks if the track types
are compatible.

Copy-Moving Objects
~~~~~~~~~~~~~~~~~~~
Holding down :zbutton:`Ctrl` while moving objects will
allow you to copy-and-move the objects to the new location.

Link-Moving Regions
~~~~~~~~~~~~~~~~~~~
Linked regions can be created by holding down :zbutton:`Alt`
while moving.

Other editing capabilities were explained previously in
:ref:`edit-modes`.
