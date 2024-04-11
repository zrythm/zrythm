.. SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

.. sectionauthor:: Alexandros Theodotou <alex@zrythm.org>

Saving & Loading
================

Loading Projects
----------------

When Zrythm launches, it will ask you to select a
project to load from a list of recent projects or
from a path, or to create a new one.

.. figure:: /_static/img/project-list.png
   :align: center

   Project selection

When you select a project, Zrythm will load it.

To load a project from a path, click :guilabel:`Open From Path...`.

Creating Projects
-----------------

To create a new project, click :guilabel:`Create New Project...`
and choose a title, parent directory and template, then click
:guilabel:`Create Project`.

.. figure:: /_static/img/create-new-project.png
   :align: center

   New project creation

.. tip:: A blank template is available, and you can create your own templates by copying a project directory under :file:`templates` in the :ref:`Zrythm user path <configuration/preferences:Zrythm User Path>`.

Saving Projects
---------------

Saving works as you would expect: :guilabel:`Save As...` will save the Project in a new location
and :guilabel:`Save` will save the Project in the
previous location.

.. important:: When saving projects, Zrythm expects you to give it a directory.

Automatic Backups
-----------------

Zrythm has an option to auto-save the current project as a back-up.
When launching Zrythm and selecting to load your project, Zrythm will
let you know if there are newer back-ups of that project and ask you
if you want to load them instead.

.. figure:: /_static/img/use-backup.png
   :align: center

   Prompt asking whether to open the found backup

.. seealso::
   See :ref:`projects/project-structure:Backups` for
   more information about backups.
