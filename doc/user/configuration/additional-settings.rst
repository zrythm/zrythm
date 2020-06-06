.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Additional Settings
===================

Zrythm stores all of its configuration using
the GSettings mechanism, which comes with the
`gsettings <https://developer.gnome.org/gio/stable/gsettings-tool.html>`_ command for changing settings
from the command line, or the optional GUI tool
`dconf-editor <https://wiki.gnome.org/Apps/DconfEditor>`_.

Normally, you shouldn't need to access any of
these settings as most of them are found inside
Zrythm's UI, and it is not recommended to
edit them as Zrythm validates some settings
before it saves them or uses some settings
internally, but in some cases you
may want to change them manually for some
reason - for example if your selected backend
causes Zrythm to crash.

Viewing the Current Settings
----------------------------

The :program:`zrythm` binary comes with a ``--print-settings`` option
(see :ref:`command-line-options` for more details) that
prints the whole configuration to the command line. In
combination with the ``--pretty`` option (ie,
``zrythm --print-settings --pretty`` or
``zrythm -p --pretty`` for short), you can get output
like the following.

.. image:: /_static/img/print-settings.png
   :align: center

This feature is mostly added for debugging purposes and
to help us identify problems with user configurations, but
feel free to use it if you are curious.

Reset to Factory Settings
-------------------------

The :program:`zrythm` binary also comes with a
``--reset-to-factory`` option for
resetting Zrythm to its default state.

.. warning:: This will clear ALL your settings.

.. note:: This will be added on the UI soon.
