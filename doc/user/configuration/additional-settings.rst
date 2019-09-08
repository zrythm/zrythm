.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

Additional Settings
===================

Zrythm stores all of its configuration using
the GSettings mechanism, which comes with the
``gsettings`` command for changing settings
from the command line, or the optional GUI tool
``dconf-editor``.

Normally, you shouldn't need to access any of
these settings as most of them are found inside
Zrythm's UI, and it is not recommended to
edit them as Zrythm validates some settings
before it saves them, but in some cases you
may want to change them manually for some
reason.

Using gsettings
---------------

To see what settings are available and for
info on how to use ``gsettings`` see ``man gsettings``.

As an example, to change the audio backend you
would do ``gsettings set org.zrythm.Zrythm.preferences audio-backend "jack"``

You can use the range option to get a list of
the available values:

::

  gsettings range org.zrythm.Zrythmpreferences audio-backend

Using dconf-editor
------------------

Install and run dconf-editor, and navigate to
``org.zrythm.Zrythm``. All the settings can be found
there.
