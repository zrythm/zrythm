.. SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

Creating Tracks
===============

Empty Tracks
------------

To add an empty track, click the :guilabel:`+` button in the tracklist
(or right click on empty space)
and select the type of track you want to add.

.. figure:: /_static/img/tracklist-add-track.png
   :align: center

   Adding a new empty track

Instantiating Plugins
---------------------

:ref:`Instantiating a plugin <plugins-files/plugins/plugin-browser:Instantiating Plugins>` will cause new tracks to be created (unless the plugin was dropped in a particular location).
If the plugin is an instrument plugin,
an instrument track will be created. If the plugin is
an effect, a bus track will be created.

.. tip:: Instrument tracks can be created this way.

Importing Files
---------------

Likewise, :ref:`importing audio/MIDI files <plugins-files/audio-midi-files/file-browser:Importing Files>`
will create audio and/or MIDI tracks containing that file.

Duplicating/Cloning
-------------------

Most Tracks can be duplicated via the :guilabel:`Duplicate` context menu option.
