.. This is part of the Zrythm Manual.
   Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

.. sectionauthor:: Alexandros Theodotou <alex@zrythm.org>

.. _command-line:

Command Line
============

Zrythm comes with a few utilities below that can be
used from the :term:`CLI`.

zrythm
------

.. program:: zrythm

.. option:: -h, --help

  Prints a list of available options.

.. option:: --pretty

  Pretty-prints the output (where applicable).

.. option:: -p, --print-settings

  Prints all the user settings. Can be combined with
  :option:`--pretty` to get pretty-printed output,
  like below.

  .. image:: /_static/img/print-settings.png
     :align: center

.. option:: --reset-to-factory

  Resets user settings to their default values.

  .. note:: Only affects the
    settings printed with :option:`-p`. This will
    *not* affect any files in the
    :term:`Zrythm user path`.

  .. warning:: This will clear ALL your user
    settings.

.. option:: -v, --version

  Prints the Zrythm version.
