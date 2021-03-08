.. This is part of the Zrythm Manual.
   Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. sectionauthor:: Alexandros Theodotou <alex@zrythm.org>

.. _preferences:

Preferences
===========

Zrythm has a Preferences dialog containing all
of the global settings that can be accessed by
clicking the gear icon or with :kbd:`Control-Shift-p`

Each section in the preferences dialog is explained
in the following sections.

General
-------

General settings.

.. image:: /_static/img/preferences-general.png
   :align: center

.. _preferences-engine:

Engine
~~~~~~

Audio engine settings.

Audio backend
  The audio backend to use. The available backends are
  JACK, RtAudio and SDL2. We recommend using :term:`JACK` when
  possible, otherwise RtAudio.
MIDI backend
  The MIDI backend to use. The available backends are
  JACK MIDI, RtMidi and WindowsMME (Windows only). We
  recommend using JACK MIDI when possible, otherwise
  RtMidi.
MIDI controllers
  A list of controllers to auto-connect to.
Audio inputs
  A list of audio inputs to auto-connect to.

Paths (Global)
~~~~~~~~~~~~~~

Global paths.

Zrythm path
  The :term:`Zrythm user path`.

Plugins
-------

:term:`Plugin` settings.

.. image:: /_static/img/preferences-plugins.png
   :align: center

UIs
~~~

Plugin UIs.

Generic UIs
  Show generic plugin UIs generated by Zrythm instead of
  custom ones.
Open UI on instantiation
  Open plugin UIs when they are instantiated.
Keep window on top
  Always show plugin UIs on top of the main window.
Bridge unsupported UIs
  Bridge unsupported UIs in another process instead of
  creating generic ones.

  .. warning:: This may lead to performance loss for some
    plugins.
Refresh rate
  Refresh rate in Hz. If set to 0, the screen's refresh rate
  will be used.

.. _vst-paths:

Paths
~~~~~

VST plugins
  The search paths to scan for :term:`VST2` plugins
  in.

  .. note:: This option is only available on Windows. On
    GNU/Linux and MacOS the paths are fixed. See
    :ref:`scanning-plugins` for details.

SFZ instruments
  The search paths to scan for :term:`SFZ`
  instruments in.
SF2 instruments
  The search paths to scan for :term:`SF2`
  instruments in.

DSP
---

:term:`DSP` settings.

.. image:: /_static/img/preferences-dsp.png
   :align: center

Pan
~~~

:term:`Panning` options for :term:`Mono` signals (not used at the moment).

Pan algorithm
  The panning algorithm to use when applying pan on mono
  signals.
  See the graph below
  for the different curves as you move the pan
  from left to right. We recommend leaving it as the
  default (Sine).
  See https://www.cs.cmu.edu/~music/icm-online/readings/panlaws/index.html
  for more information.

  .. figure:: /_static/img/pan_algorithms.png
     :figwidth: image
     :align: center

     Pan algorithms (:blue:`sine`,
     :red:`square root`, :green:`linear`).

Pan law
  The :term:`Pan law` to use when applying pan on
  mono signals.

Editing
-------

Editing options.

.. image:: /_static/img/preferences-editing.png
   :align: center

Audio
~~~~~

Audio editing.

Fade algorithm
  Default fade algorithm to use for fade in/outs.

Automation
~~~~~~~~~~

Automation editing.

Curve algorithm
  Default curve algorithm to use for automation
  curves.

Undo
~~~~

Undo options.

Undo stack length
  Maximum undo history stack length. Set to -1 for
  unlimited.

  .. note:: We recommend leaving it at 128.


Projects
--------

Project settings.

.. image:: /_static/img/preferences-projects.png
   :align: center

General (Project)
~~~~~~~~~~~~~~~~~

General project settings.

Autosave interval
  Interval to auto-save projects, in minutes.
  Auto-saving will be disabled if this is set to 0.

  .. note:: This refers to automatic back-ups. The main project
    will not be overwritten unless you explicitly save it.

UI
--

User interface options.

.. image:: /_static/img/preferences-ui.png
   :align: center

General (UI)
~~~~~~~~~~~~

General user interface options.

User interface language
  The language to use for the user interface.

  .. hint:: For a list of supported languages and their
    translation status, or to translate Zrythm into
    your language, see
    `Weblate <https://hosted.weblate.org/projects/zrythm/>`_.

.. note:: Some of these settings require a restart of Zrythm
  to take effect.