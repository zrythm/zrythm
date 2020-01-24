.. This is part of the Zrythm Manual.
   Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
   See the file index.rst for copying conditions.

GNU with Linux
==============

.. _gnu-with-linux-installer:

Installer
---------
You can install the latest version of
Zrythm for your distro using an installer
from the
`Download <https://www.zrythm.org/en/download.html>`_ page
on our website.

The installer supports the following
distros (and derivatives):

- **Ubuntu 18.04 & 19.04** - amd64
- **LinuxMint 19.3** - amd64
- **Debian 9 & 10** - amd64
- **Arch Linux** - x86_64
- **Fedora 31** - x86_64
- **OpenSUSE Tumbleweed** - x86_64

This is the easiest and recommended way to install
the latest version for most users, and we provide
support for it.

Distro Package
--------------
If your distribution provides a Zrythm package you
can install it from there, but it will likely be an older
version than the one we provide and it may be missing
optimizations or features that we include with the
installer.

.. _gnu-with-linux-manual-installation:

Manual Installation
-------------------
If you are an advanced user and prefer to build Zrythm
yourself, Zrythm uses the Meson build system, so the
procedure to build and install is as follows:

::

  meson build
  ninja -C build
  ninja -C build install

See the ``INSTALL.md`` file for more information.
