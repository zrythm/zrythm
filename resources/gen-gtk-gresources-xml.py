#!/usr/bin/env python3
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

for c in ['zrythm', 'gnome-builder']:
  icons_dir = 'icons/{0}'.format(c)
  if os.path.exists(os.path.join(srcdir,icons_dir)):
    for f in get_files(icons_dir, '.svg'):
      xml += '    <file>icons/{0}/{1}</file>\n'.format(c,f)


xml += '''
  <file>theme.css</file>
  </gresource>
</gresources>'''

if len(sys.argv) > 2:
  outfile = sys.argv[2]
  f = open(outfile, 'w')
  f.write(xml)
  f.close()
