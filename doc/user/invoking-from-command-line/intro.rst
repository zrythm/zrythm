.. SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
   This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. sectionauthor:: Alexandros Theodotou <alex@zrythm.org>

.. _command-line:

Command Line
============

Zrythm comes with a few utilities below that can be
used from the :term:`CLI`.

zrythm
------
Zrythm executable.

.. program:: zrythm

.. cmdoption:: -h, --help

  Print a list of available options.

.. cmdoption:: --pretty

  Pretty-print the output (where applicable).

.. cmdoption:: -p, --print-settings

  Print all the user settings. Can be combined with
  :option:`zrythm --pretty` to get pretty-printed output,
  like below.

  .. image:: /_static/img/print-settings.png
     :align: center

.. cmdoption:: --reset-to-factory

  Reset user settings to their default values.

  .. note:: Only affects the
    settings printed with :option:`zrythm -p`. This will
    *not* affect any files in the
    :term:`Zrythm user path`.

  .. warning:: This will clear ALL your user
    settings.

.. cmdoption:: -v, --version

  Print version information.

.. cmdoption:: --zpj-to-yaml

  Convert a .zpj project file to YAML.

.. cmdoption:: --yaml-to-zpj

  Convert a YAML file to the .zpj format.

  Prints the Zrythm version.

zrythm_launch
-------------
Wrapper over :program:`zrythm` that sets the
correct paths before launching Zrythm. All of the
options for :program:`zrythm` can also be passed to
:program:`zrythm_launch`.

.. program:: zrythm_launch
