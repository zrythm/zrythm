.. SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

Additional Settings
===================

GSettings
---------
Zrythm stores most of its configuration using
the GSettings mechanism, which comes with the
``gsettings`` command for changing settings
from the command line, or the optional GUI tool
`dconf-editor <https://wiki.gnome.org/Apps/DconfEditor>`_.

.. warning::
   Normally, you shouldn't need to access any of
   these settings as most of them are found inside
   Zrythm's UI, and it is not recommended to
   edit them as Zrythm validates some settings
   before it saves them or uses some settings
   internally.

Viewing the Current Settings
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

See :option:`zrythm --print-settings`.

Reset to Factory Settings
~~~~~~~~~~~~~~~~~~~~~~~~~

See :option:`zrythm --reset-to-factory` and
:ref:`configuration/preferences:Reset to Factory Settings`.

Plugin Settings
---------------
Located at :file:`plugin-settings.yaml` under the
:term:`Zrythm user path`, this is a collection of
per-plugin settings, such as whether to open the
plugin with :term:`Carla`, the
:ref:`bridge mode <plugins-files/plugins/plugin-window:Opening Plugins in Bridged Mode>`
to use and whether to use a
:ref:`generic UI <plugins-files/plugins/plugin-window:Generic UIs>`.

Zrythm will remember the last setting used for each
plugin and automatically apply it when you choose to
instantiate that plugin from the
:doc:`plugin browser <../plugins-files/plugins/plugin-browser>`.

.. note:: This file is generated and maintained
   automatically by Zrythm and users are not
   expected to edit it.
