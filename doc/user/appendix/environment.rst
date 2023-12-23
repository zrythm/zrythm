.. SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
.. SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. sectionauthor:: Alexandros Theodotou <alex@zrythm.org>

.. _environment:

Environment Variables
=====================

Zrythm understands the following environment variables. They can be
configured in :file:`.bashrc` or a similar shell configuration file, the
operating system's settings, or they can be passed directly when running
:program:`zrythm` or :program:`zrythm_launch`, like:
``ZRYTHM_SKIP_PLUGIN_SCAN=1 zrythm_launch``

.. envvar:: ZRYTHM_SKIP_PLUGIN_SCAN

  Set to 1 to disable plugin scanning.

  Example:
  ``ZRYTHM_SKIP_PLUGIN_SCAN=1``

.. envvar:: ZRYTHM_DSP_THREADS

  Number of DSP threads to use. Defaults to number
  of CPU cores - 1.

.. envvar:: ZRYTHM_DEBUG

  Set to 1 to show extra information useful for
  developers.
