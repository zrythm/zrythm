.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Saving & Loading
================

Loading Projects
----------------

When Zrythm launches, it will ask you to select a project to load from
a list of recent projects,
or to create a new one.

.. image:: /_static/img/project_list.png
   :align: center

When you select a project and click :guilabel:`Apply`, Zrythm will
load that project.

Creating Projects
-----------------

In the menu above, if you select :guilabel:`Create new project`,
Zrythm will ask you for a template to use for creating the new
Project.

.. image:: /_static/img/project_select_template.png
   :align: center

.. tip:: A blank template is available, and you can
  create your own templates by copying a project
  directory to :file:`{ZRYTHM_USER_DIR}/templates`.

Once a template (or blank) is selected and you click :guilabel:`Apply`,
Zrythm will ask you for a parent directory to save the Project
in and a title for the Project.

.. image:: /_static/img/create_new_project.png
   :align: center

Once you accept, the new Project will be set up and you will be
ready to go.

Saving Projects
---------------

Saving works as you would expect: :guilabel:`Save As...` will save the Project in a new location
and :guilabel:`Save` will save the Project in the
previous location.

.. note:: When saving projects, Zrythm expects you to
  give it a directory

Automatic Backups
-----------------

Zrythm has an option to auto-save the current project as a back-up.
When launching Zrythm and selecting to load your project, Zrythm will
let you know if there are newer back-ups of that project and ask you
if you want to load them instead.

.. image:: /_static/img/use_backup.png
   :align: center

The auto-save interval can be changed (or disabled) in the preferences
window.

.. tip:: The backups are saved in the project's directory, under :file:`backups`.
