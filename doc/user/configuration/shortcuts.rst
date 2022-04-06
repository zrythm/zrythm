.. SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
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
    schema_version: 2
    shortcuts:
      - action: app.manual
        primary: F1
      - action: app.open
        primary: <Control>o
      - action: app.goto-prev-marker
        primary: KP_4 # keypad 4
        secondary: Backspace
    ...

.. todo:: Make this more user friendly & document all
   actions.
