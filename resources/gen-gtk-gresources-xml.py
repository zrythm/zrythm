#!/usr/bin/env python3
#
#  Copyright (C) 2018-2019 Alexandros Theodotou
#
#  This file is part of Zrythm
#
#  Zrythm is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Affero General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  Zrythm is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Affero General Public License for more details.
#
#  You should have received a copy of the GNU Affero General Public License
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
  it under the terms of the GNU Affero General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Zrythm is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
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
  <gresource prefix='/org/zrythm/Zrythm/app'>
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
  'application-ogg',
  'application-msword',
  'applications-internet',
  'application-x-m4',
  'audio-midi',
  'audio-mp3',
  'audio-flac',
  'audio-speakers-symbolic',
  'audio-headphones',
  'audio-x-wav',
  'audio-card',
  'configure-shortcuts',
  'document-properties',
  'draw-line',
  'dialog-messages',
  'document-new',
  'document-duplicate',
  'document-open',
  'document-save',
  'document-save-as',
  'document-send',
  'draw-eraser',
  'delete',
  'distortionfx',
  'edit-select',
  'emblem-symbolic-link',
  'edit-select-symbolic',
  'editor',
  'edit-undo',
  'edit-redo',
  'edit-cut',
  'edit-copy',
  'edit-paste',
  'edit-delete',
  'edit-select-all',
  'edit-clear',
  'edit-duplicate',
  'favorites',
  'format-justify-fill',
  'gnumeric-column-hide',
  'gtk-quit',
  'gtk-add',
  'hand',
  'help-about',
  'help-donate',
  'help-contents',
  'inode-directory',
  'kdenlive-show-audiothumb',
  'kt-show-statusbar',
  'kdenlive-snap',
  'kdenlive-show-markers',
  'labplot-transform-move',
  'media-seek-backward',
  'media-seek-forward',
  'media-playlist-repeat',
  'media-record',
  'media-playback-start',
  'media-playback-stop',
  'media-optical-audio',
  'media-album-track',
  'minuet-chords',
  'media-repeat-album-amarok',
  'mathmode',
  'music-note-16th',
  'news-subscribe',
  'node-type-cusp',
  'none',
  'object-locked',
  'object-unlocked',
  'online',
  'offline',
  'plugins',
  'step_object_Controller',
  'selection-end-symbolic',
  'select-rectangular',
  'text-x-csrc',
  'tools-report-bug',
  'view-fullscreen',
  'visibility',
  'window-close',
  'window-close-symbolic',
  'window-maximize',
  'window-minimize',
  'window-pin',
  'zoom-in',
  'zoom-out',
  'zoom-fit-best',
  'zoom-original',
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
