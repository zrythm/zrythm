.. This is part of the Zrythm Manual.
   Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. sectionauthor:: Alexandros Theodotou <alex@zrythm.org>

Glossary
========

.. glossary::

  a2jmidid
    a2jmidid is an application that bridges between
    the system MIDI ports and :term:`JACK`.

  Balance
    Balance (or :term:`stereo` balance) is the
    left-right balance of a stereo audio signal.

  DAW
    A :abbr:`DAW (Digital Audio Workstation)` is
    software used to compose, record, edit, arrange,
    mix and master music.

  DSP
    In the context of a :term:`DAW`,
    :abbr:`DSP (Digital Signal Processing)` means
    audio signal processing.

  Free software
    Free/libre software means users control the
    program, as opposed to proprietary software,
    where the program controls the users.

  JACK
    :abbr:`JACK (JACK Audio Connection Kit)` is a
    sound
    server that offers low latency for users and
    provides a simple API for developers. It is the
    de facto standard sound server for professional
    audio on GNU/Linux.

  LV2
    :abbr:`LV2 (LADSPA Version 2)` is an extensible,
    cross-platform open standard for audio
    plugins. LV2 has a simple core interface, which
    is accompanied by extensions that add more
    advanced functionality.

  MIDI
    :abbr:`MIDI (Musical Instrument Digital Interface)`
    is a technical standard for communication
    between musical instruments and computers.

  MIDI note
    :term:`MIDI` Notes are used to trigger virtual
    (or hardware) instruments.

    .. image:: /_static/img/midi_note.png
       :align: center

  Mono
    A single-channel audio signal (see also
    :term:`stereo`).

  Panning
    Panning is the distribution of a :term:`mono`
    signal
    into a new stereo or multi-channel sound field.

  Plugin
    An audio plugin is a piece software that
    performs audio
    processing (for example an audio effect or an
    instrument) that can be used inside plugin
    hosts, such as a :term:`DAW`.

  Range
    A Range is a selection of time between two
    positions.

    .. image:: /_static/img/ranges.png
       :align: center

  Region
    A Region (Clip) is a container for MIDI Notes,
    audio or other events. Regions can be repeated,
    like below.

    .. image:: /_static/img/region.png
       :align: center

    The content of regions can be edited in an
    editor (see :ref:`editors`).

  Stereo
    A two-channel audio signal (left and right) (see
    also :term:`mono`).

  VST2
    A proprietary plugin standard and the predecessor
    of VST3. We do not recommend using this standard.

  VST3
    VST3 is a plugin standard that supersedes
    :term:`VST2`. It is better to use VST3 than VST2,
    because it is released as :term:`free software`.
    However, we recommend using the :term:`LV2`
    standard instead.

  Zrythm user path
    The path where Zrythm will save user data,
    such as projects, temporary files, presets and
    exported audio.
    The default is :file:`zrythm` under

    * :envvar:`XDG_DATA_HOME` (see the
      `XDG Base Directory Specification <https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html>`_)
      on freedesktop-compliant systems (or if
      :envvar:`XDG_DATA_HOME` is defined), or

    * the directory for local application data on
      Windows
