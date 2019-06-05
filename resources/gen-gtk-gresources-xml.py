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

xml = '''<!--
  Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>

  This file is part of Zrythm

  Zrythm is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Zrythm is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
-->
<?xml version='1.0' encoding='UTF-8'?>
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
  <gresource prefix='/org/zrythm/app'>
'''

xml += '\n'

for f in get_files('ui', '.ui'):
  xml += '    <file preprocess=\'xml-stripblanks\'>ui/{0}</file>\n'.format(f)

xml += '\n'

# add icons
for c in ['zrythm', 'gnome-builder', 'ext', 'fork-awesome', 'font-awesome']:
  icons_dir = 'icons/{0}'.format(c)
  if os.path.exists(os.path.join(srcdir,icons_dir)):
    for f in get_files(icons_dir, '.svg'):
      xml += '    <file>icons/{0}/{1}</file>\n'.format(c,f)
    for f in get_files(icons_dir, '.png'):
      xml += '    <file>icons/{0}/{1}</file>\n'.format(c,f)

def remove_prefix(text, prefix):
  if text.startswith(prefix):
    return text[len(prefix):]
  return text

icons_dir = 'icons/breeze-icons'
_breeze_icons = [
  'window-minimize',
  'media-record',
  'media-playback-start',
  'media-playback-stop',
  'media-optical-audio',
  'document-properties',
  'media-seek-backward',
  'media-seek-forward',
  'media-playlist-repeat',
  'draw-line',
  'audio-speakers-symbolic',
  'application-msword',
  'zoom-in',
  'zoom-out',
  'zoom-fit-best',
  'zoom-original',
  'node-type-cusp',
  'text-x-csrc',
  'audio-headphones',
  'visibility',
  'view-fullscreen',
  'format-justify-fill',
  'audio-midi',
  'audio-mp3',
  'audio-flac',
  'application-ogg',
  'dialog-messages',
  'audio-x-wav',
  'inode-directory',
  'none',
  'audio-card',
  'minuet-chords',
  'emblem-symbolic-link',
  'window-close',
  'window-close-symbolic',
  'edit-select-symbolic',
  'editor',
  'document-new',
  'document-open',
  'document-save',
  'document-save-as',
  'document-send',
  'application-x-m4',
  'gtk-quit',
  'edit-undo',
  'edit-redo',
  'edit-cut',
  'edit-copy',
  'edit-paste',
  'edit-delete',
  'edit-select-all',
  'kt-show-statusbar',
  'edit-clear',
  'select-rectangular',
  'media-repeat-album-amarok',
  'help-about',
  'help-donate',
  'help-contents',
  'configure-shortcuts',
  'tools-report-bug',
  'delete',
  'edit-duplicate',
  'applications-internet',
  'draw-eraser',
  'step_object_Controller',
  'gtk-add',
  'kdenlive-show-markers',
  'document-duplicate',
  'mathmode',
  'distortionfx',
  'selection-end-symbolic',
  'edit-select',
  'plugins',
  'hand',
  'window-maximize',
  ]
for cat in ['actions', 'animations', 'applets', 'apps', 'categories', 'devices', 'emblems', 'emotes', 'mimetypes', 'places', 'preferences', 'status']:
  for size in ['12', '16', '22', '24', '32', '64', 'symbolic']:
    src_dir = '{0}/{1}'.format(cat,size)
    alias_dir = ''
    if (size == 'symbolic'):
      alias_dir = '16x16/{0}'.format(cat)
    else:
      alias_dir = '{0}x{0}/{1}'.format(size,cat)
    full_src_dir = os.path.join(icons_dir,src_dir)
    full_alias_dir = os.path.join(icons_dir,alias_dir)
    if os.path.exists(os.path.join(srcdir,full_src_dir)):
      for icon_name in _breeze_icons:
        for f in get_files(full_src_dir, '.svg'):
          if (f == (icon_name + '.svg')):
            icon_path = os.path.join(full_src_dir,f)
            alias = os.path.join(full_alias_dir,'z-' + f)
            xml += '    <file alias=\"{0}\">{1}</file>\n'.format(alias,remove_prefix(icon_path, srcdir + os.sep))

xml += '''
  <file>theme.css</file>
  </gresource>
</gresources>'''

if len(sys.argv) > 2:
  outfile = sys.argv[2]
  f = open(outfile, 'w')
  f.write(xml)
  f.close()
