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

See :option:`zrythm --print-settings`.

Reset to Factory Settings
-------------------------

See :option:`zrythm --reset-to-factory`.

.. note:: This will be added to the UI soon.
