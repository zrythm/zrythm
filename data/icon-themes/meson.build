# SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
    'TRACK_TYPES_DIR': get_option ('datadir') / 'zrythm/themes/icons/zrythm-dark/scalable/tracktypes',
    'CMD1': os_windows ? 'cp' : 'ln',
    'CMD2': os_windows ? '-f' : '-s',
    'CMD3': os_windows ? '' : '-f',
    })

# create symlinks/copies for track icons
meson.add_install_script (
  meson.current_build_dir () / 'install_tracktype_icons.sh')

# install app icon
install_data (
  'zrythm-dark/scalable/apps/zrythm.svg',
  install_dir:
    datadir / 'icons/hicolor/scalable/apps')
