.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Overview
========

Tracks are the main building blocks of projects.
Tracks appear in the tracklist and contain various
events such as regions.

.. image:: /_static/img/track.png
   :align: center

Most types of tracks have a corresponding channel that
appears in the mixer.

.. image:: /_static/img/channel.png
   :align: center

Each track has its own page in the inspector section, which is
shown when selecting a Track.

.. image:: /_static/img/track_inputs.png
   :align: center

There are various kinds of Tracks suited for
different purposes, explained in the following
sections.

.. important:: In Zrythm, group tracks are used for
   grouping signals (direct routing), FX/bus tracks are
   used for effects/sends, and folder tracks are used for
   putting tracks under the same folder and doing
   common operations. This is different from what many
   other DAWs do, so please keep this in mind.

   If you have used Cubase before, you might find this easy
   to get used to.

Track Interface
---------------

.. image:: /_static/img/track-interface.png
   :align: center

Each track has a color, an icon (corresponding to its type)
a name and various buttons. Tracks that can have
lanes, like instrument tracks, will also have an option to
display each lane as above. Tracks that can have automation
will have an option to display automation tracks as above.

If the track produces output, it will have a meter on its
right-hand side showing the current level.

Track Inspector
----------------

.. image:: /_static/img/midi-track-inspector.png
   :align: center

Each track will display its page in the inspector when
selected. Depending on the track, the properties shown
will be differ.

Properties
~~~~~~~~~~

Track properties are basic properties that the track
has, like its name, color, and direct output.

Inputs
~~~~~~

If the track takes input, there will be an input
selection section.

Sends
~~~~~

These are ports that can be connected to for any
purpose. These are generally useful for side-chaining.
The pre-fader port(s) hold the signal up to before
the fader is processed, and the post-fader port(s)
hold the signal after the fader is applied.

Context Menu
------------

Each tracks has a context menu, with additional options
depending on its type.

.. image:: /_static/img/track-context-menu-duplicate-track.png
   :align: center

The section :ref:`track-operations` explains the
various track operations available.
