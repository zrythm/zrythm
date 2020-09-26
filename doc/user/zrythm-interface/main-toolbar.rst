.. This is part of the Zrythm Manual.
   Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. _main-toolbar:

Main Toolbar
============

The main toolbar contains global actions, such as saving
and loading a project or opening the preferences window.

.. image:: /_static/img/main-toolbar.png
   :align: center

About Dialog
------------

Clicking the Zrythm icon will show the About dialog, which
contains information about authors and copyright.

.. image:: /_static/img/about-dialog.png
   :align: center

Global Menus
------------
Zrythm has the following global menus at the top of its
interface.

Edit Menu
~~~~~~~~~
The Edit menu various buttons and controls that are used
often during editing and arranging.

The :ref:`editing` chapter explains these controls
in detail.

Project Menu
~~~~~~~~~~~~
The Project menu contains various project-related
actions such as saving, loading and exporting MIDI
or audio.

New Project
+++++++++++
Create a new project.

Save
++++
Save the current project at its current location.

Save As
+++++++
Save the current project at a new location.

Load
++++
Load a project.

Export As
+++++++++
Export the project as audio or MIDI. See
:ref:`export-audio-and-midi` for more details.

Export Graph
++++++++++++
Export the routing graph as an image or .dot graph.
See :ref:`export-routing-graph` for more details.

For more information about projects, see the
:ref:`projects` chapter.

View Menu
~~~~~~~~~
The View menu contains controls to change the appearance of
Zrythm and its various areas, such as zooming.

Zoom In
  Zooms in.

Zoom Out
  Zooms out.

Original Size
  Zooms back to the default zoom level.

Best Fit
  Zooms in or out as much as required to show all of the
  events in the timeline.

Toggle Left/Right/Bottom Panels
  Toggles the panel's visibility.

Toggle Timeline Visibility
  Toggles the timeline's visibility.

Help Menu
~~~~~~~~~
The Help menu contains links for reporting bugs, donating,
chatting and other useful links.

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

Additional Controls
-------------------
The main toolbar also contains the following widgets.

.. image:: /_static/img/main-toolbar-right-side.png
   :align: center

There are the following buttons next to the name of the
current project.

Scripting Interface
  Shows the scripting interface. See :ref:`scripting` for more
  information.
Log Viewer
  Displays the log, which gets updated real-time.
Preferences
  Shows the preferences dialog. See :ref:`preferences` for more
  information.

The MIDI In widget shows the :term:`MIDI` activity
of auto-connected MIDI devices and the live waveform
display shows the audio waveform from the master
output.
