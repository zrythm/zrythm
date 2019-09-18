.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Project Info
============

In Zrythm, your work is saved inside a Project.
Projects consist of a folder with a YAML
file describing the Project and additional
files used such as MIDI and audio files.

Project Structure
-----------------

::

    ~/zrythm/Projects/myproject
    ├── backups
    │   ├── myproject.bak
    │   │   ├── project.yml
    │   │   └── states
    │   │       └── Calf Organ_0
    │   │           ├── Calf Organ.ttl
    │   │           └── manifest.ttl
    │   └── myproject.bak1
    │       ├── project.yml
    │       └── states
    │           └── Calf Organ_0
    │               ├── Calf Organ.ttl
    │               └── manifest.ttl
    ├── exports
    │   └── mixdown.FLAC
    ├── pool
    │   └── perfect_kick_body_5.wav.wav
    └── project.yml

backups
  Regular backups taken automatically by Zrythm
myproject.bak
  An example backup project
states
  Contains plugin states at the time of save (for
  LV2)
exports
  Default directory for exporting files related to
  the project
pool
  Used by Zrythm to store and load files
  used by the Project, such as audio files
project.yml
  The Project file
