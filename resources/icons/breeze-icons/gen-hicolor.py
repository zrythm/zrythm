#!/usr/bin/env python3
#
#  Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
#
#  This file is part of Zrythm
#
#  Zrythm is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  Zrythm is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
#
# Generate gtk.gresources.xml
#
# Usage: gen-gtk-gresources-xml SRCDIR_GTK [OUTPUT-FILE]

import os, sys, shutil

srcdir = sys.argv[1]

def get_files(subdir,extension):
  return sorted(filter(lambda x: x.endswith((extension)), os.listdir(os.path.join(srcdir,subdir))))

def assure_path_exists(path):
  dir = os.path.dirname(path)
  if not os.path.exists(dir):
    os.makedirs(dir)

def delete_file_if_exists(filepath):
  if os.path.exists(filepath):
    os.remove(filepath)

for size in ['12x12', '16x16', '22x22', '24x24', '32x32', '64x64']:
  path = os.path.join(sys.argv[2],size)
  if os.path.exists(path):
    shutil.rmtree(path)

for cat in ['actions', 'animations', 'applets', 'apps', 'categories', 'devices', 'emblems', 'emotes', 'mimetypes', 'places', 'preferences', 'status']:
  for size in ['12', '16', '22', '24', '32', '64', 'symbolic']:
    dirpath = '{0}/{1}'.format(cat,size)
    if size == 'symbolic':
      destdir = '16x16/{0}'.format(cat)
    else:
      destdir = '{0}x{0}/{1}'.format(size,cat)
    if os.path.exists(os.path.join(srcdir,dirpath)):
      for f in get_files(dirpath, '.svg'):
        zf = "z-" + f
        destfile = os.path.join(sys.argv[2],destdir,zf)
        assure_path_exists(destfile)
        delete_file_if_exists(destfile)
        shutil.copy(os.path.join(srcdir,dirpath,f),destfile)

