#!/usr/bin/env python3
#
# Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
#
# This file is part of Zrythm
#
# Zrythm is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Zrythm is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.

import os
import subprocess
from shutil import which

prefix = os.environ.get('MESON_INSTALL_PREFIX', '/usr/local')
datadir = os.path.join(prefix, 'share')

schemadir = os.path.join(datadir, 'glib-2.0', 'schemas')
fontsdir = os.path.join(datadir, 'fonts', 'zrythm')
desktop_database_dir = os.path.join(datadir, 'applications')

if not os.environ.get('DESTDIR'):
    print('Compiling gsettings schemas...')
    subprocess.call(['glib-compile-schemas', schemadir])
    print('Updating font cache...')
    subprocess.call(['fc-cache', fontsdir])
    print('Updating icon cache...')
    subprocess.call(['touch', datadir + '/icons/hicolor'])
    subprocess.call(['gtk-update-icon-cache'])
    print('Updating desktop database...')
    if not os.path.exists(desktop_database_dir):
        os.makedirs(desktop_database_dir)
    subprocess.call(['update-desktop-database', '-q', desktop_database_dir])

    if which('update-gdk-pixbuf-loaders') is not None:
        subprocess.call(['update-gdk-pixbuf-loaders'])
