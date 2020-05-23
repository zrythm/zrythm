# Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

gschema_top = 'org.zrythm.Zrythm'
gschema = configure_file (
  input: gschema_gen_wrap,
  output: gschema_top + '.gschema.xml',
  command: [
    '@INPUT0@', '@OUTPUT@', gschema_top,
    ],
  install: true,
  install_dir: schemasdir)

# install fonts if needed
if get_option('dseg_font')
  install_subdir (
    join_paths ('fonts', 'DSEG14-Classic-MINI'),
    install_dir: fontsdir)
endif

#install samples
install_data (
  'samples/klick/square_emphasis.wav',
  install_dir: samplesdir)
install_data (
  'samples/klick/square_normal.wav',
  install_dir: samplesdir)

# install desktop file and launcher script
if os_gnu or os_freebsd
  zrythm_launcher_config = configuration_data ()
  zrythm_launcher_config.set (
    'SCHEMAS_DIR', schemasdir)
  zrythm_launcher_config.set (
    'BIN_DIR', bindir)
  zrythm_launcher = configure_file (
    input: 'zrythm_launch.in',
    output: 'zrythm_launch',
    configuration: zrythm_launcher_config,
    install: true,
    install_dir: bindir,
    install_mode: 'r-xr-xr-x')

  desktop_file = configure_file (
    input: 'zrythm.desktop.in',
    output: 'zrythm.desktop.in',
    configuration: zrythm_launcher_config)
  localized_desktop_file = i18n.merge_file (
    input: desktop_file,
    output: 'zrythm.desktop',
    type: 'desktop',
    po_dir: '../po',
    install: true,
    install_dir: join_paths (
      datadir,
      'applications'),
  )
  desktop_utils = find_program (
    'desktop-file-validate', required: false)
  if desktop_utils.found()
    test('Validate desktop file', desktop_utils,
      args: [localized_desktop_file]
    )
  endif
endif

# install icon theme
install_subdir (
  'icon-themes/zrythm-dark',
  install_dir: themesdir / 'icons')

# install app icon
install_data (
  'icon-themes/zrythm-dark/scalable/apps/zrythm.svg',
  install_dir:
    datadir / 'icons/hicolor/scalable/apps')

# Zrythm project MIME type
install_data (
  'org.zrythm.Zrythm-mime.xml',
  install_dir: datadir / 'mime/packages')

if not os_windows
  install_data (
    'zrythm-completion.bash',
    install_dir: join_paths (
      sysconfdir, 'bash_completion.d'),
    install_mode: 'r--r--r--',
    rename: 'zrythm')
endif

# GtkSourceView styles
install_data (
  'gtksourceview-monokai-extended.xml',
  install_dir: sourceviewstylesdir,
  rename: 'monokai-extended-zrythm.xml')

# theme
install_data (
  'theme.css',
  install_dir: themesdir,
  rename: 'zrythm-theme.css')

subdir('windows')
