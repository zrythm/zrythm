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

GNU/Linux
=========

Official Builds
---------------
You can install the latest version of Zrythm for your distro `here <https://software.opensuse.org//download.html?project=home%3Aalextee&package=zrythm>`_

You will be presented with the following

.. image:: /_static/img/software_opensuse.png

This is the recommended way to install the latest version for
most users.

Flatpak
-------
Flatpak builds are coming soon.

AppImage
--------
AppImage builds are coming soon.

..
  KX Studio
  ----------
  Thanks to falktx, Zrythm will also be available in the `KX Studio repos <http://kxstudio.linuxaudio.org/>`_ for Debian users

..
  LibraZik
  --------
  For French speaking users, Zrythm is fully translated and
  available in `LibraZik <https://librazik.tuxfamily.org/>`_.

AUR
---
For Arch GNU/Linux users, Zrythm is also available in the AUR
under `zrythm <https://aur.archlinux.org/packages/zrythm/>`_
and `zrythm-git <https://aur.archlinux.org/packages/zrythm-git/>`_.

Manual Installation
-------------------
Zrythm uses the Meson build system, so the
procedure to build and install is as follows:

::

  meson build
  ninja -C build
  ninja -C build install

See the ``meson.options`` file for installation options.
