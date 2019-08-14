.. Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>

   This file is part of Zrythm

   Zrythm is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   Zrythm is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU General Affero Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.

Creating Tracks
===============

Blank Tracks
------------

To add an empty track, right click on empty space in the
Tracklist and select the type of track you want to add.

Creating Tracks From Plugins
----------------------------

Plugins can be clicked and dragged from the Plugin Browser
and dropped into empty space in the Tracklist or Mixer to
instantiate them. If the plugin is an Instrument plugin,
an Instrument Track will be created. If the plugin is
an effect, a Bus Track will be created.

Creating Audio Tracks From Audio Files
--------------------------------------

Likewise, to create an Audio Track from an audio file
(WAV, FLAC, etc.), you can drag an audio file from the
File Browser into empty space in the Tracklist or Mixer.
This will create an Audio Track containing a single
Audio Clip at the current Playhead position.

Duplicating Tracks
==================

Most Tracks can be duplicated by right clicking
inside the Track and selecting :zbutton:`Duplicate`.
