#!/usr/bin/env python3
#
#  Copyright (C) 2018-2019 Alexandros Theodotou
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

import os, sys

srcdir = sys.argv[1]

xml = '''<?xml version='1.0' encoding='UTF-8'?>
<gresources>
  <gresource prefix='/org/gtk/libgtk'>
'''

def get_files(subdir,extension):
  return sorted(filter(lambda x: x.endswith((extension)), os.listdir(os.path.join(srcdir,subdir))))

xml += '''
    <file>theme/Matcha-dark-sea/gtk.css</file>
    <file>theme/Matcha-dark-sea/gtk-dark.css</file>
'''

for f in get_files('theme/Matcha-dark-sea/assets', '.png'):
  xml += '    <file>theme/Matcha-dark-sea/assets/{0}</file>\n'.format(f)

xml += '\n'

for f in get_files('theme/Matcha-dark-sea/assets', '.svg'):
  xml += '    <file>theme/Matcha-dark-sea/assets/{0}</file>\n'.format(f)

xml += '''
  </gresource>
  <gresource prefix='/org/zrythm'>
'''

xml += '\n'

for f in get_files('ui', '.ui'):
  xml += '    <file preprocess=\'xml-stripblanks\'>ui/{0}</file>\n'.format(f)

xml += '\n'

# add icons
for c in ['zrythm', 'gnome-builder']:
  icons_dir = 'icons/{0}'.format(c)
  if os.path.exists(os.path.join(srcdir,icons_dir)):
    for f in get_files(icons_dir, '.svg'):
      xml += '    <file>icons/{0}/{1}</file>\n'.format(c,f)

def remove_prefix(text, prefix):
  if text.startswith(prefix):
    return text[len(prefix):]
  return text

icons_dir = os.path.join(srcdir,'icons/breeze-icons')
for path, subdirs, files in os.walk(icons_dir):
  for name in files:
    if (name.endswith('window-minimize.svg') or
          name.endswith('emblem-symbolic-link.svg') or
          name.endswith('audio-card.svg') or
          name.endswith('window-close.svg') or
          name.endswith('media-record.svg') or
          name.endswith('media-playback-start.svg') or
          name.endswith('media-playback-stop.svg') or
          name.endswith('media-seek-backward.svg') or
          name.endswith('media-seek-forward.svg') or
          name.endswith('media-playlist-repeat.svg') or
          name.endswith('window-close-symbolic.svg') or
          name.endswith('edit-select.svg') or
          name.endswith('editor.svg') or
          name.endswith('draw-eraser.svg') or
          name.endswith('draw-line.svg') or
          name.endswith('audio-speakers-symbolic.svg') or
          name.endswith('application-msword.svg') or
          name.endswith('gtk-add.svg') or
          name.endswith('window-maximize.svg')):
      xml += '    <file>{0}</file>\n'.format(remove_prefix(os.path.join(path, name), srcdir + os.sep))

xml += '''
  <file>theme.css</file>
  </gresource>
</gresources>'''

if len(sys.argv) > 2:
  outfile = sys.argv[2]
  f = open(outfile, 'w')
  f.write(xml)
  f.close()
