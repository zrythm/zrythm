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
import shutil

prefix = os.environ.get('MESON_INSTALL_PREFIX', '/usr/local')
datadir = os.path.join(prefix, 'share')

schemadir = os.path.join(datadir, 'glib-2.0', 'schemas')
fontsdir = os.path.join(datadir, 'fonts', 'zrythm')
desktop_database_dir = os.path.join(datadir, 'applications')
mime_dir = os.path.join(datadir, 'mime')
docdir = os.path.join(datadir, 'doc', 'zrythm')

if not os.environ.get('DESTDIR'):
    print('Compiling gsettings schemas...')
    subprocess.call(['glib-compile-schemas', schemadir])
    print('Updating font cache...')
    subprocess.call(['fc-cache', fontsdir])
    print('Updating icon cache...')
    subprocess.call(['touch', datadir + '/icons/hicolor'])
    subprocess.call(['gtk-update-icon-cache'])
    print('Updating MIME database...')
    subprocess.call([
        'update-mime-database', mime_dir])
    print('Updating desktop database...')
    if not os.path.exists(desktop_database_dir):
        os.makedirs(desktop_database_dir)
    subprocess.call(['update-desktop-database', '-q', desktop_database_dir])

    if shutil.which('update-gdk-pixbuf-loaders') is not None:
        subprocess.call(['update-gdk-pixbuf-loaders'])

# create hard links for fonts for user manual to
# make size smaller
# symlinks don't work because of CORS
en_parent_dir = os.path.join(docdir, 'en')
for lang in ['de', 'fr', 'ja', 'pt', 'pt_BR',
        'nb_NO']:
    # dir containing en, fr, de, etc.
    parent_dir = os.path.join(docdir, lang)
    fonts_dir = os.path.join (parent_dir, '_static', 'fonts')
    en_fonts_dir = os.path.join (en_parent_dir, '_static', 'fonts')
    if (os.path.exists(fonts_dir)):
        print ("recreating {}".format(fonts_dir))
        shutil.rmtree(fonts_dir)
        os.mkdir(fonts_dir)
    print ("linking {}".format(fonts_dir))
    for dirpath, dirnames, filenames in os.walk(en_fonts_dir):
        for filename in filenames:
            localdir = dirpath.replace(en_fonts_dir, '')
            if (len(localdir) > 0 and
                    localdir[0] == '/'):
                localdir = localdir[1:]
            os.makedirs(
                os.path.join(
                    fonts_dir, localdir),
                exist_ok=True)
            os.link(
                os.path.join(
                    en_fonts_dir, localdir,
                    filename),
                os.path.join(
                    fonts_dir, localdir,
                    filename))
