.. Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>

   This file is part of Zrythm

   Zrythm is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   Zrythm is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU General Affero Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.

Additional Settings
===================

Zrythm stores all of its configuration using
the GSettings mechanism, which comes with the
``gsettings`` command for changing settings
from the command line.

Normally, you shouldn't need to access any of
these settings as most of them are found inside
Zrythm's UI, and it is not recommended to
edit them as Zrythm validates some settings
before it saves them, but in some cases you
may want to change them manually for some
reason.

To see what settings are available and for
info on how to use ``gsettings`` see ``man gsettings``.

As an example, to change the audio backend you
would do ``gsettings set org.zrythm.preferences audio-backend "jack"``

You can use the range option to get a list of
the available values:

::

  gsettings range org.zrythm.preferences audio-backend
