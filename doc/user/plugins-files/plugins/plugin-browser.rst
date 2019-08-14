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

Plugin Browser
==============

The Plugin Browser makes it easy to browse and
filter Plugins installed on your computer.

.. image:: /_static/img/plugin_browser.png

Filters Tab
-----------
The filters tab at the top allows you to select how to filter
the Plugins. There are 3 tabs:

Collection
  This tab contains your collections. You can create
  collections such as "MySynths" and filter by the selected
  collections.
Category
  Filter Plugins by category based on the plugin metadata,
  such as "Delay", "Distortion", etc.
Protocol
  Allows you to filter plugins based their protocol (LV2 or
  VST). Not operational at the moment.

Filter Buttons
--------------
Additionally to the above, you can filter plugins based on
their type. The following types exist:

Instrument
  These plugins will create an instrument track when added
  to the project.
Effects
  These plugins can be dragged into the insert slots of
  existing channels in the mixer or can be instantiated to
  create new bus tracks.
Modulators
  These plugins output CV signals and can be used to modulate
  other plugin or track parameters.
MIDI Effects
  These plugins modify incoming MIDI signals and can be used
  in the MIDI signal chain of an Instrument or MIDI track
  (coming soon).

Instantiating Plugins
---------------------
There are a couple of ways to instantiate plugins:

Drag-n-Drop
  Drag and drop the selected plugin into empty space in the
  Tracklist or into empty space in the Mixer to
  create a new track using that plugin. If the plugin is a
  modulator, you can drop it into the Modulators Tab.
Double Click/Enter
  Double click on the plugin or select it and press the
  return key on your keyboard to create a new track using
  that plugin.
