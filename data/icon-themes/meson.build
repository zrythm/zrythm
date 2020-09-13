# Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

theme_iconsdir = themesdir / 'icons'

# install zrythm dark icon theme
install_subdir (
  'zrythm-dark',
  install_dir: theme_iconsdir)

install_icons = configure_file (
  output: 'install_tracktype_icons.sh',
  input: 'install_tracktype_icons.sh.in',
  configuration: {
    'BASH': bash.full_path (),
    'TRACK_TYPES_DIR': theme_iconsdir / 'zrythm-dark' / 'scalable' / 'tracktypes',
    'CMD1': os_windows ? 'cp' : 'ln',
    'CMD2': os_windows ? '' : '-s',
    })

# create symlinks/copies for track icons
meson.add_install_script (
  meson.current_build_dir () / 'install_tracktype_icons.sh')

# install app icon
install_data (
  'zrythm-dark/scalable/apps/zrythm.svg',
  install_dir:
    datadir / 'icons/hicolor/scalable/apps')
