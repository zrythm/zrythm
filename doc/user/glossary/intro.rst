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

  AU
    A proprietary plugin standard, mainly used on
    MacOS. We recommend using :term:`LV2` instead.

  Balance
    Balance (or :term:`stereo` balance) is the
    left-right balance of a stereo audio signal.

  CLI
    :abbr:`CLI (Command Line Interface)` is the
    interface users interact with when typing
    commands in a terminal.

  Clip
    Used interchangeably with
    :ref:`region <regions>`.

  Control
    A control is a :doc:`port <../routing/ports>`
    with a user-changeable control value that
    affects a processor, such as a plugin. For
    example, a gain control. Used interchangeably
    with :term:`parameter`.

  CV
    In a software environment,
    :abbr:`CV (Control Voltage)` represents
    audio-rate control data.

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

  Pan Law
    TODO

  Panning
    Panning is the distribution of a :term:`mono`
    signal
    into a new stereo or multi-channel sound field.

  Parameter
    Used interchangeably with :term:`control`.

  Plugin
    A plugin is an external module that provides
    audio processing capabilities to Zrythm, such
    as an :term:`SFZ` instrument or an :term:`LV2`
    reverb plugin.

  Project
    A project refers to a work session. It is saved
    as a directory containing a project file along
    with other auxiliary files. See :ref:`Projects`
    for more details.

  Range
    A Range is a selection of time between two
    positions.

    .. image:: /_static/img/ranges.png
       :align: center

  Region
    A region (or :term:`clip`) is a container for
    MIDI Notes, audio or other events. See
    :ref:`regions`.

  SFZ
    SFZ is a file format for sample-based
    virtual instruments.

  SF2
    SF2 is the successor of :term:`SFZ`.

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
