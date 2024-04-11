.. SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
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

When you select a project and click
:guilabel:`&Open Selected`, Zrythm will load that
project.

To load a project from a path, click
:guilabel:`Open from Path`.

Creating Projects
-----------------

To create a new project, select the
:guilabel:`Create New` tab and choose a template
for creating the new project, then click
:guilabel:`Create`.

.. figure:: /_static/img/project-select-template.png
   :align: center

   Project template selection

.. tip:: A blank template is available, and you can
  create your own templates by copying a project
  directory under :file:`templates` in the
  :ref:`Zrythm user path <configuration/preferences:Zrythm User Path>`.

After clicking :guilabel:`Create`, Zrythm will ask
you for a parent directory to save the project in
and a title for the project.

.. figure:: /_static/img/create-new-project.png
   :align: center

   New project creation

Once you click :guilabel:`OK`, the new project will
be set up and you will be ready to go.

Saving Projects
---------------

Saving works as you would expect: :guilabel:`Save As...` will save the Project in a new location
and :guilabel:`Save` will save the Project in the
previous location.

.. important:: When saving projects, Zrythm expects
   you to give it a directory

Automatic Backups
-----------------

Zrythm has an option to auto-save the current project as a back-up.
When launching Zrythm and selecting to load your project, Zrythm will
let you know if there are newer back-ups of that project and ask you
if you want to load them instead.

.. image:: /_static/img/use_backup.png
   :align: center

.. seealso::
   See :ref:`projects/project-structure:Backups` for
   more information about backups.
