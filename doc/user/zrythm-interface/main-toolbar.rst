.. This is part of the Zrythm Manual.
   Copyright (C) 2020-2022 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _main-toolbar:

Main Toolbar
============

The main toolbar contains global actions, such as
saving and loading a project or opening the
preferences window.

.. image:: /_static/img/main-toolbar.png
   :align: center

The name of the project is displayed at the center,
with the project path being shown below it.

Global Menus
------------
The main toolbar contains the following global menus.

Edit Menu
~~~~~~~~~
The Edit menu has various buttons and controls that
are used often during editing and arranging.

The :ref:`editing` chapter explains these controls
in detail.

Project Menu
~~~~~~~~~~~~
The Project menu contains various project-related
actions such as saving, loading and exporting MIDI
or audio.

Project Management
++++++++++++++++++
New Project
  Create a new project.
Save
  Save the current project at its current location.
Save As
  Save the current project at a new location.
Load
  Load a project.

Export
++++++
Export As
  Export the project as audio or MIDI. See
  :ref:`export-audio-and-midi` for more details.
Export Graph
  Export the routing graph as an image or .dot graph.
  See :ref:`export-routing-graph` for more details.

.. seealso:: For more information about projects, see
   the :ref:`projects` chapter.

View Menu
~~~~~~~~~
The View menu contains controls to change the
appearance of Zrythm and its various areas.

Toggle Status Bar
  Toggles the visibility of the status bar.
Fullscreen
  Toggles fullscreen mode.
Toggle Left/Right/Bottom/Main Panels
  Toggles the visiblity of the selected panel.

Help Menu
~~~~~~~~~
The Help menu contains links for reporting bugs,
donating, chatting and other useful links.

About
  View information about the running instance of
  Zrythm, such as version information, authors and
  copyright.

  .. image:: /_static/img/about-dialog.png
     :align: center
Chat
  Join the Zrythm chatroom on Matrix.
Manual
  View the user manual.
News
  Show the latest changelog.
Keyboard Shortcuts
  Show all the available keyboard shortcuts.
Donate
  Donate to Zrythm through LiberaPay.
Report a Bug
  Opens the page to report a new bug.

Additional Buttons
------------------
The main toolbar also contains the following buttons.

.. image:: /_static/img/main-toolbar-right-buttons.png
   :align: center

Scripting Interface
  Shows the :ref:`scripting interface <scripting>`.
Log Viewer
  Displays the log, which gets updated real-time.
Preferences
  Shows the :ref:`preferences dialog <preferences>`.

Live Indicators
---------------

The following live indicators are displayed on the
right.

MIDI In
  Shows the :term:`MIDI` activity of auto-connected
  MIDI devices.
Oscilloscope
  Shows the audio waveform from the master output.
