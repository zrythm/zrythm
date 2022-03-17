.. This is part of the Zrythm Manual.
   Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. sectionauthor:: Alexandros Theodotou <alex@zrythm.org>

.. _preferences:

Preferences
===========

Zrythm has a Preferences dialog containing all
of the global settings that can be accessed by
clicking the gear icon or with :kbd:`Control-Shift-p`

.. figure:: /_static/img/preferences-general.png
   :align: center

   Preferences dialog

Preferences are grouped by sections, such as
:guilabel:`General` and :guilabel:`Plugins`, and
each item in the preferences contains a tooltip
that explains what it does.

.. note:: Zrythm must be restarted for changes to
   take effect.

Searching
---------

The preferences are searchable. You can simply start
typing a keyword and results will appear
automatically. Clicking on one of the results will
navigate to the corresponding item in the
preferences.

.. figure:: /_static/img/preferences-searching.png
   :align: center

   Searching the user preferences

Notable Preferences
-------------------

Below is a list of notable preference items with
some comments about them.

Backend Selection
~~~~~~~~~~~~~~~~~

The audio/MIDI backend in under
:menuselection:`General --> Engine` controls which
audio engine implementation Zrythm will use to
receive input from and send output to devices.
:term:`JACK`/JACK MIDI is the recommended backend
combination on all systems.

Input Devices/Controllers
~~~~~~~~~~~~~~~~~~~~~~~~~

In order to be able to record input from MIDI/audio
devices, the devices must be explicitly enabled
using the :guilabel:`MIDI controllers` and
:guilabel:`Audio inputs` settings
under :menuselection:`General --> Engine`.

Zrythm will ignore input from any devices not
explicitly enabled.

Undo Stack Length
~~~~~~~~~~~~~~~~~

The size of the undo stack, found under
:menuselection:`Editing --> Undo` controls how
far back you can undo user actions in a given
project. For example, if set to 128, you can go
back 128 (undoable) user actions.

.. hint:: The undo history is saved inside
   Zrythm projects, so you can still undo even after
   you load the project in a new instance of Zrythm.

.. warning:: Setting this to a very large number will
   increase the size of project files and it may
   take longer to save/load projects. We recommend
   leaving it to the default value.

Backup Saving
~~~~~~~~~~~~~

Zrythm can save backups automatically while working
on a project. You can specify the interval to do
this, in minutes, under
:menuselection:`Projects --> General`. To disable
auto-save, set the interval to 0.

User Interface Language
~~~~~~~~~~~~~~~~~~~~~~~

The language of the user interface can be changed
under :menuselection:`UI --> General`.
