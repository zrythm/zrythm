.. SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. _file-browser:

File Browser
============

The File Browser makes it easy to navigate
through files on your computer or through
your custom collections.

.. figure:: /_static/img/file-browser.png
   :align: center

   Built-in file browser

Bookmarks
---------
The Bookmarks section contains a list of bookmarked
directories. Double-clicking on a directory will
show its contents in the file list at the bottom.

Predefined Bookmarks
~~~~~~~~~~~~~~~~~~~~
Zrythm provides a predefined bookmark for the user's :file:`Home` directory.

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
:ref:`plugin browser <plugins-files/plugins/plugin-browser:Plugin Browser>`,
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
  Only show preset files (currently unused/unimplemented).

Searching
~~~~~~~~~

Files can be filtered by a search term from the provided search box.

Auditioning
-----------
Audio and MIDI files can be auditioned by making
use of the auditioning controls at the bottom of the
file list.

.. figure:: /_static/img/file-auditioning-controls.png
   :align: center

   File auditioning controls

Play
  Trigger playback of the selected file.
Stop
  Stop playback of the selected file.
Volume
  Control the loudness of the output audio.

Instrument
~~~~~~~~~~
The selected instrument will be used for playing
back MIDI files. In the above example, the `amsynth`
instrument is selected.

Preferences
-----------

Clicking the cog button on the right side of the auditioning controls will
bring up the file browser preferences screen. The following settings are
available.

Autoplay
  Automatically play back the selected file when the selection changes
Show Unsupported Files
  By default, only supported files will be shown in
  the file list. Enabling this option will also show unsupported files.
Show Hidden Files
  By default, only files that are not hidden will be
  shown in the file list. Enabling this option will also show hidden files.

File Info
---------
Information, such as metadata, for the selected file
is always shown at the bottom of the file panel.

.. figure:: /_static/img/file-info-label.png
   :align: center

   Information about the selected audio file

Importing Files
---------------

There are various ways to import MIDI and audio files into a project.

Drag and Drop From a File Browser
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can drag one or more files from your system's file browser or from the
built-in file browser into a track (if applicable), into empty space in the timeline or
into empty space in the tracklist or mixer.

Droping onto an existing track will
import the file on that track (if possible), and dropping onto empty space
will create one or more new tracks (depending on the number of dropped files, or number of tracks contained in Type 1 MIDI files).

.. hint:: You can adjust the location where the dropped file will
   be inserted in the project by dropping at an appropriate spot

.. figure:: /_static/img/file-drop-in-track.png
   :align: center

   Dropping one or more audio files into an existing audio track

.. figure:: /_static/img/file-drop-below-track.png
   :align: center

   Dropping a file at a specific location in a new track

.. figure:: /_static/img/file-drop-in-tracklist.png
   :align: center

   Dropping a file at empty space in the tracklist

Activating a File From the Built-in File Browser
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A file (row) can be activated by selecting a row and pressing the return key
or double-clicking it.

Tracklist/Mixer Context Menu
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Files can also be imported by right-clicking in empty space in the tracklist
or mixer and selecting :guilabel:`Import File...`

.. figure:: /_static/img/right-click-import-file.png
   :align: center

   Importing a file via the tracklist context menu
