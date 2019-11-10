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

install_data (
  'org.zrythm.Zrythm.gschema.xml',
  install_dir: schemasdir)

# install fonts if needed
if get_option('install_dseg_font')
  install_subdir (
    join_paths ('fonts', 'DSEG14-Classic-MINI'),
    install_dir: fontsdir)
endif

#install samples
install_data (
  join_paths (
    'samples', 'klick', 'square_emphasis.wav'),
  install_dir: samplesdir)
install_data (
  join_paths (
    'samples', 'klick', 'square_normal.wav'),
  install_dir: samplesdir)

# Desktop launcher and description file.
desktop_file = i18n.merge_file (
  input: 'zrythm.desktop.in',
  output: 'zrythm.desktop',
  type: 'desktop',
  po_dir: '../po',
  install: true,
  install_dir: join_paths (
    datadir,
    'applications'),
)

# Zrythm project MIME type
install_data (
  'org.zrythm.Zrythm-mime.xml',
  install_dir:
    join_paths (
      [datadir, 'mime/packages']))

desktop_utils = find_program (
  'desktop-file-validate', required: false)
if desktop_utils.found()
  test('Validate desktop file', desktop_utils,
    args: [desktop_file]
  )
endif
