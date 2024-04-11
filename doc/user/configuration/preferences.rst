.. SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. sectionauthor:: Alexandros Theodotou <alex@zrythm.org>

.. _preferences:

Preferences
===========

Zrythm has a Preferences dialog containing all
of the global settings that can be accessed by
clicking the gear icon or with :kbd:`Control-,`

.. figure:: /_static/img/preferences-general.png
   :align: center

   Preferences dialog

Preferences are grouped by sections, such as
:guilabel:`General` and :guilabel:`Plugins`, and
each item in the preferences contains a description
that explains what it does (when not self-explanatory).

.. important:: Zrythm must be restarted for some changes to take effect.

.. note:: Zrythm stores these preferences using
   the GSettings mechanism, which comes with the
   ``gsettings`` command for changing settings
   from the command line, or the optional GUI tool
   `dconf-editor <https://wiki.gnome.org/Apps/DconfEditor>`_.

   Normally, you shouldn't need to access any of
   these settings this way and it is not recommended to
   edit them this way as Zrythm validates some settings
   before it saves them (Zrythm also uses some settings
   internally), but this can be useful for debugging problems.

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

Below is a list of notable preference items with some comments about them.

Zrythm User Path
~~~~~~~~~~~~~~~~

This is the path where Zrythm will save user data, such as projects, temporary
files, presets and exported audio. The default is :file:`zrythm` under

* :envvar:`XDG_DATA_HOME` (see the
  `XDG Base Directory Specification <https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html>`_)
  on freedesktop-compliant systems (or if
  :envvar:`XDG_DATA_HOME` is defined), or

* ``%LOCALAPPDATA%`` on Windows

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

Zrythm will ignore input from any devices not explicitly enabled.

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

   Consider this functionality temporary/short-term. Do not rely long-term on this functionality as Zrythm may drop the undo history when upgrading projects to newer versions (due to compatibility reasons, or to fix bugs).

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

Reset to Factory Settings
-------------------------

There is an option to reset Zrythm to its default
settings under :menuselection:`General --> Other`.

.. seealso:: This behaves similarly to :option:`zrythm --reset-to-factory` on the command line.
