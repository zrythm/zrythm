.. SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
   This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. sectionauthor:: Alexandros Theodotou <alex@zrythm.org>

Running Zrythm
==============

Initial Configuration
---------------------

When you first run Zrythm, it will display a dialog
that lets you configure the basic settings that
Zrythm will use. These include the
:term:`Zrythm user path` and the language of the
user interface.

.. figure:: /_static/img/initial-configuration-dialog.png
   :align: center

   Initial configuration dialog

Language
  Zrythm lets you choose the language of the
  interface. The interface is already translated in
  `multiple languages <https://hosted.weblate.org/projects/zrythm/#languages>`_,
  so choose the language you are most comfortable in.

  .. note:: You must have a locale enabled for the
    language you want to use.
Path
  This is the :term:`Zrythm user path`.

.. tip:: More settings are
   available in the :ref:`preferences`.

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

.. figure:: /_static/img/project-list.png
   :align: center

   Project selection

.. figure:: /_static/img/first-run-interface.png
   :align: center

   Main interface
