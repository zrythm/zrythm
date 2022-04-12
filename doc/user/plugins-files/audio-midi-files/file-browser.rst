.. SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. _file-browser:

File Browser
============

The File Browser makes it easy to navigate
through files on your computer or through
your custom collections.

.. image:: /_static/img/file-browser.png
   :align: center

Bookmarks
---------
The Bookmarks section contains a list of bookmarked
directories. Double-clicking on a directory will
show its contents in the file list below.

Standard Bookmarks
~~~~~~~~~~~~~~~~~~
Zrythm provides standard bookmarks for
:file:`Home` and :file:`Desktop`, which correspond
to the home and desktop directories of the current
user.

Custom Bookmarks
~~~~~~~~~~~~~~~~
Custom bookmarks can be added by right-clicking on
a directory in the file list and selecting
:guilabel:`Add Bookmark`.

.. image:: /_static/img/file-browser-add-bookmark.png
   :align: center

Similarly, custom bookmarks can be removed by
right-clicking on the bookmark and selecting
:guilabel:`Delete`.

.. image:: /_static/img/file-browser-delete-bookmark.png
   :align: center

Filtering
---------

Filter Buttons
~~~~~~~~~~~~~~
Similarly to the
:doc:`plugin browser <../plugins/plugin-browser>`,
files can be filtered based on their type.

.. figure:: /_static/img/file-filter-buttons.png
   :align: center

   Filtering by audio files

The following toggles are available.

Audio
  Only show audio files.
MIDI
  Only show MIDI files.
Presets
  Only show preset files.

.. todo:: Preset filtering is not implemented yet.

Showing Unsupported Files
~~~~~~~~~~~~~~~~~~~~~~~~~
By default, only supported files will be shown in
the file list. To also show unsupported files,
enable :guilabel:`Show unsupported files` in the
:ref:`file browser preferences <plugins-files/audio-midi-files/file-browser:Preferences>`.

Showing Hidden Files
~~~~~~~~~~~~~~~~~~~~
By default, only files that are not hidden will be
shown in the file list. To also show hidden files,
enable :guilabel:`Show hidden files` in the
:ref:`file browser preferences <plugins-files/audio-midi-files/file-browser:Preferences>`.

Auditioning
-----------
Audio and MIDI files can be auditioned by making
use of the auditioning controls at the bottom of the
file list.

.. figure:: /_static/img/file-auditioning-controls.png
   :align: center

   File auditioning controls

|icon_media-playback-start| Play
  Trigger playback of the selected file.
|icon_media-playback-stop| Stop
  Stop playback of the selected file.
Volume
  Control the loudness of the output audio.

Instrument
~~~~~~~~~~
The selected instrument will be used for playing
back MIDI files. In the above example, the `Helm`
instrument is selected.

Autoplay
~~~~~~~~
Auto-play (automatically playing back the selected
file when the selection changes) can be enabled in
the preferences below.

Preferences
-----------

Clicking the cog icon will bring up the preferences
for the file browser.

.. figure:: /_static/img/file-browser-preferences.png
   :align: center

   File browser preferences

File Info
---------
Information, such as metadata, for the selected file
is always shown at the bottom of the file panel.

.. figure:: /_static/img/file-info-label.png
   :align: center

   Information about the selected audio file

Importing Files
---------------
Files are imported into the project by either
double-clicking or dragging and dropping into the
timeline.

.. figure:: /_static/img/file-drag-n-drop.png
   :align: center

   Drag-n-dropping an audio file into a new track

Pop-up File Browser
-------------------
An additional file browser with similar
functionalities is available by clicking the
:guilabel:`Popup File Browser` button near the bottom
right of the main window.

.. image:: /_static/img/popup-file-browser.png
   :align: center
