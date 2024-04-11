.. SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

Project Structure
=================

In Zrythm, work is saved inside
:term:`projects <Project>`.
Projects consist of a directory with a project
file describing the project along with additional
files and directories used by the project, such as
audio files and plugin states.

Backups
-------
Backups are also kept in the project directory,
and each backup is a standalone project
on its own - ie, it contains all the files it needs
to load as a separate project.

Projects inside the :file:`backups` dir of a
parent project are considered backups of the
parent project.

.. hint::
  Zrythm will keep backups automatically by default
  during autosave.
  You can change the autosave interval or turn it off
  in the
  :ref:`preferences <configuration/preferences:Preferences>`.

Project Directory
-----------------

.. code-block:: tree

    ~/zrythm/Projects/myproject # Project directory
    ├── backups # Project backups
    │   ├── myproject.bak
    │   │   ├── exports
    │   │   │   └── stems
    │   │   ├── plugins
    │   │   │   ├── ext_file_copies
    │   │   │   ├── ext_file_links
    │   │   │   └── states
    │   │   │       ├── LSP Multi-Sampler x12 Stereo_IDSDZ0
    │   │   │       ├── Vital_QSQHZ0
    │   │   │       ├── Vital_TPMNZ0
    │   │   │       └── Vital_Y3N3Y0
    │   │   └── f,project.zpj
    │   └── myproject.bak1
    │       ├── exports
    │       │   └── stems
    │       ├── plugins
    │       │   ├── ext_file_copies
    │       │   ├── ext_file_links
    │       │   └── states
    │       │       ├── LSP Multi-Sampler x12 Stereo_IDSDZ0
    │       │       ├── Vital_QSQHZ0
    │       │       ├── Vital_TPMNZ0
    │       │       └── Vital_Y3N3Y0
    │       └── f,project.zpj
    ├── exports # Project exports
    │   ├── stems
    │   └── f,mixdown.FLAC
    ├── plugins # Plugin states
    │   ├── ext_file_copies
    │   ├── ext_file_links
    │   │   ├── l,lo-fi-cow.wav
    │   │   ├── l,sn1.wav
    │   │   ├── l,sn2.wav
    │   │   ├── l,sn3.wav
    │   │   └── l,sn4.wav
    │   └── states
    │       ├── LSP Multi-Sampler x12 DirectOut_0NVNZ0
    │       │   ├── l,lo-fi-cow.wav
    │       │   ├── f,manifest.ttl
    │       │   ├── l,sn1.wav
    │       │   ├── l,sn2.wav
    │       │   ├── l,sn3.wav
    │       │   ├── l,sn4.wav
    │       │   └── f,state.ttl
    │       ├── LSP Multi-Sampler x12 DirectOut_G7EBZ0
    │       │   ├── f,manifest.ttl
    │       │   └── f,state.ttl
    │       ├── LSP Multi-Sampler x12 Stereo_IDSDZ0
    │       │   └── f,state.carla
    │       ├── LSP Multi-Sampler x24 Stereo_XC88Y0
    │       │   ├── f,manifest.ttl
    │       │   └── f,state.ttl
    │       ├── Rubber Band Stereo Pitch Shifter_1NPNZ0
    │       │   └── f,state.carla
    │       ├── ToTape6_8MNJZ0
    │       │   └── f,state.carla
    │       ├── Vital_9S87Y0
    │       │   └── f,state.carla
    │       └── Vital_Y3N3Y0
    │           └── f,state.carla
    ├── pool # Audio file pool
    │   ├── f,perfect_kick_body_5.wav
    │   ├── f,Audio Track - lane 1 - recording.wav
    │   └── f,Audio Track - lane 2 - recording.wav
    └── f,project.zpj # Project file

Project File Format
-------------------
Zrythm project files (``project.zpj``) are
`zstd <https://facebook.github.io/zstd/>`_-compressed
`JSON <https://www.json.org/>`_ files (the project file
`MIME type <https://en.wikipedia.org/wiki/MIME>`_
``x-zrythm-project`` is an alias of ``application/zstd``).

Project files can be converted to JSON using:

.. prompt:: bash

   zstd -d project.zpj -o project.json

Audio Pool
----------
The audio pool (:file:`pool` directory) contains all
audio files currently used by the project. Zrythm
will automatically delete unused files during save.

.. note:: These files will be copied to backups as
   well.

Project Compatibility
---------------------

Project format is not expected to change in a breaking way. When we make
changes to the project format, Zrythm will auto-convert old projects to the
new format.
