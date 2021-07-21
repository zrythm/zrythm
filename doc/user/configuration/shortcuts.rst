.. This is part of the Zrythm Manual.
   Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Shortcuts
=========

Shortcuts (key bindings) can be previewed by pressing
:kbd:`Control-?` or :kbd:`Control-F1`.

Global Shortcuts
----------------

.. image:: /_static/img/global_shortcuts.png
   :align: center

Editor Shortcuts
----------------

.. image:: /_static/img/editor_shortcuts.png
   :align: center

Custom Shortcuts
----------------

Shortcuts can be overridden by placing
a file named :file:`shortcuts.yaml` under the
:term:`Zrythm user path`.

An example is given below.

.. code-block:: yaml

    ---
    schema_version: 1
    shortcuts:
      - action: app.manual
        shortcut: F1
      - action: app.open
        shortcut: <Control>o
    ...

.. todo:: Make this more user friendly & document all
   actions.
