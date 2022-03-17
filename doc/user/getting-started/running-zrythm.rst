.. This is part of the Zrythm Manual.
   Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. sectionauthor:: Alexandros Theodotou <alex@zrythm.org>

Running Zrythm
==============

First Run Wizard
----------------

When you first run Zrythm, it will display a wizard
that lets you configure the basic settings that
Zrythm will use. These include the
:term:`Zrythm user path`, the language of the
user interface and audio/MIDI backends.

Language Selection
~~~~~~~~~~~~~~~~~~

.. image:: /_static/img/first-run-language.png
   :align: center

Zrythm lets you choose the language of the
interface. The interface is already translated in
`multiple languages <https://hosted.weblate.org/projects/zrythm/#languages>`_,
so choose the language you are most comfortable in.

.. note:: You must have a locale enabled for the
  language you want to use.

Path
~~~~

.. image:: /_static/img/first-run-path.png
   :align: center

This is the :term:`Zrythm user path`.

Audio/MIDI Backends
~~~~~~~~~~~~~~~~~~~

.. image:: /_static/img/first-run-backends.png
   :align: center

Zrythm supports multiple audio and MIDI backend
engines. :term:`JACK` is the recommended one for
both.

If JACK is not available on your system, we
recommend RtAudio and RtMidi.

Click :guilabel:`Test` to try connecting to the
backend to see if it is functional.

.. note:: If you choose to use JACK, JACK MIDI and
   the JACK audio backend must be chosen together.

.. _midi_devices:

.. tip:: All of the settings mentioned here are also
   available in the :ref:`preferences`, so don't
   worry if you selected the wrong settings.

Plugin Scan
-----------
When the first run wizard is completed, Zrythm will
start
:doc:`scanning for plugins <../plugins-files/plugins/scanning>`.

.. image:: /_static/img/splash-plugin-scan.png
   :align: center

Project Selection
-----------------
Finally, Zrythm will ask you to
:doc:`load or create a project <../projects/saving-loading>`
and then the
:doc:`main interface <../zrythm-interface/overview>`
will show up.

.. figure:: /_static/img/project_list.png
   :align: center

   Project selection

.. figure:: /_static/img/first-run-interface.png
   :align: center

   Main interface
