.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Creating Tracks
===============

Blank Tracks
------------

To add an empty track, right click on empty space in the
Tracklist and select the type of track you want to add.

.. image:: /_static/img/tracklist-context-menu.png
   :align: center

Creating Tracks From Plugins
----------------------------

Plugins can be clicked and dragged from the Plugin Browser
and dropped into empty space in the Tracklist or Mixer to
instantiate them. If the plugin is an instrument plugin,
an instrument Track will be created. If the plugin is
an effect, a bus Track will be created.

See :ref:`instantiating-plugins` for how to instantiate
plugins.

Creating Audio Tracks From Audio Files
--------------------------------------

Likewise, to create an Audio Track from an audio file
(WAV, FLAC, etc.), you can drag an audio file from the
File Browser into empty space in the Tracklist or Mixer.
This will create an Audio Track containing a single
Audio Clip at the current Playhead position.

Creating Tracks by Duplicating
------------------------------

Most Tracks can be duplicated by right clicking
inside the Track and selecting :zbutton:`Duplicate`.

.. image:: /_static/img/track-context-menu-duplicate-track.png
   :align: center
