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
  <gresource prefix='/org/zrythm'>
'''

xml += '\n'

for f in get_files('ui', '.ui'):
  xml += '    <file preprocess=\'xml-stripblanks\'>ui/{0}</file>\n'.format(f)

xml += '\n'

# add icons
for c in ['zrythm', 'gnome-builder', 'ext']:
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
for cat in ['actions', 'animations', 'applets', 'apps', 'categories', 'devices', 'emblems', 'emotes', 'mimetypes', 'places', 'preferences', 'status']:
  for size in ['12', '16', '22', '24', '32', '64']:
    destdir = '{0}x{0}/{1}'.format(size,cat)
    full_destdir = os.path.join(icons_dir,destdir)
    if os.path.exists(os.path.join(srcdir,full_destdir)):
      for f in get_files(full_destdir, '.svg'):
        destfile = os.path.join(full_destdir,f)
        if (f.endswith('window-minimize.svg') or
              f.endswith('emblem-symbolic-link.svg') or
              f.endswith('audio-card.svg') or
              f.endswith('window-close.svg') or
              f.endswith('media-record.svg') or
              f.endswith('media-playback-start.svg') or
              f.endswith('media-playback-stop.svg') or
              f.endswith('media-optical-audio.svg') or
              f.endswith('document-properties') or
              f.endswith('media-seek-backward.svg') or
              f.endswith('media-seek-forward.svg') or
              f.endswith('media-playlist-repeat.svg') or
              f.endswith('window-close-symbolic.svg') or
              f.endswith('edit-select.svg') or
              f.endswith('editor.svg') or
              f.endswith('draw-eraser.svg') or
              f.endswith('draw-line.svg') or
              f.endswith('audio-speakers-symbolic.svg') or
              f.endswith('application-msword.svg') or
              f.endswith('gtk-add.svg') or
              f.endswith('zoom-in.svg') or
              f.endswith('zoom-out.svg') or
              f.endswith('zoom-fit-best.svg') or
              f.endswith('zoom-original.svg') or
              f.endswith('node-type-cusp.svg') or
              f.endswith('text-x-csrc.svg') or
              f.endswith('document-properties.svg') or
              f.endswith('visibility.svg') or
              f.endswith('view-fullscreen.svg') or
              f.endswith('format-justify-fill.svg') or
              f.endswith('media-optical-audio.svg') or
              f.endswith('audio-midi.svg') or
              f.endswith('audio-mp3.svg') or
              f.endswith('audio-flac.svg') or
              f.endswith('application-ogg.svg') or
              f.endswith('dialog-messages.svg') or
              f.endswith('audio-x-wav.svg') or
              f.endswith('inode-directory.svg') or
              f.endswith('none.svg') or
              f.endswith('audio-card.svg') or
              f.endswith('emblem-symbolic-link.svg') or
              f.endswith('window-minimize.svg') or
              f.endswith('window-close.svg') or
              f.endswith('window-close-symbolic.svg') or
              f.endswith('edit-select-symbolic.svg') or
              f.endswith('editor.svg') or
              f.endswith('document-new.svg') or
              f.endswith('document-open.svg') or
              f.endswith('document-save.svg') or
              f.endswith('document-save-as.svg') or
              f.endswith('document-send.svg') or
              f.endswith('application-x-m4.svg') or
              f.endswith('gtk-quit.svg') or
              f.endswith('edit-undo.svg') or
              f.endswith('edit-redo.svg') or
              f.endswith('edit-cut.svg') or
              f.endswith('edit-copy.svg') or
              f.endswith('edit-paste.svg') or
              f.endswith('edit-delete.svg') or
              f.endswith('edit-select-all.svg') or
              f.endswith('kt-show-statusbar.svg') or
              f.endswith('edit-clear.svg') or
              f.endswith('select-rectangular.svg') or
              f.endswith('media-repeat-album-amarok.svg') or
              f.endswith('help-about.svg') or
              f.endswith('help-donate.svg') or
              f.endswith('help-contents.svg') or
              f.endswith('configure-shortcuts.svg') or
              f.endswith('tools-report-bug.svg') or
              f.endswith('delete.svg') or
              f.endswith('edit-duplicate.svg') or
              f.endswith('applications-internet.svg') or
              f.endswith('draw-eraser.svg') or
              f.endswith('draw-line.svg') or
              f.endswith('audio-speakers-symbolic.svg') or
              f.endswith('step_object_Controller.svg') or
              f.endswith('application-msword.svg') or
              f.endswith('gtk-add.svg') or
              f.endswith('document-duplicate.svg') or
              f.endswith('distortionfx.svg') or
              f.endswith('selection-end-symbolic.svg') or
              f.endswith('edit-select.svg') or
              f.endswith('plugins.svg') or
              f.endswith('hand.svg') or
              f.endswith('window-maximize.svg')):
          xml += '    <file>{0}</file>\n'.format(remove_prefix(destfile, srcdir + os.sep))
    full_destdir = os.path.join(icons_dir,destdir)

xml += '''
  <file>theme.css</file>
  </gresource>
</gresources>'''

if len(sys.argv) > 2:
  outfile = sys.argv[2]
  f = open(outfile, 'w')
  f.write(xml)
  f.close()
