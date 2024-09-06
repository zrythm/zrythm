# SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

theme_iconsdir = themesdir / 'icons'

# install zrythm dark icon theme
install_subdir (
  'zrythm-dark',
  install_dir: theme_iconsdir)

# create symlinks/copies for track icons
meson.add_install_script (
  python3, 'install_tracktype_icons.py',
  get_option ('datadir') / 'zrythm/themes/icons/zrythm-dark/scalable/tracktypes',
  os_windows ? 'cp' : 'ln',
  )

# install app icon
install_data (
  'zrythm-dark/scalable/apps/zrythm.svg',
  rename: 'org.zrythm.Zrythm.svg',
  install_dir: datadir / 'icons/hicolor/scalable/apps')
