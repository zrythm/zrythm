# Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

app_id = 'org.zrythm.Zrythm'

gschema = configure_file (
  input: gen_gschema_wrap,
  output: app_id + '.gschema.xml',
  command: [
    '@INPUT0@', '@OUTPUT@', app_id,
    ],
  install: true,
  install_dir: schemasdir)

# install fonts if needed
if get_option('dseg_font')
  install_subdir (
    'fonts/DSEG14-Classic-MINI',
    install_dir: fontsdir)
endif

#install samples
install_data (
  'samples/klick/square_emphasis.wav',
  install_dir: samplesdir)
install_data (
  'samples/klick/square_normal.wav',
  install_dir: samplesdir)

zrythm_launcher_config = configuration_data ()
zrythm_launcher_config.set (
  'SCHEMAS_DIR', schemasdir)
zrythm_launcher_config.set ('BIN_DIR', bindir)
zrythm_launcher_config.set (
  'PROGRAM_NAME_LOWERCASE', prog_name_lowercase)
zrythm_launcher_config.set (
  'ZRYTHM_LIBDIR', zrythm_libdir)
zrythm_launcher_config.set (
  'ZRYTHM_CARLA_LIBDIR', zrythm_libdir / 'carla')
zrythm_launcher_config.set ('LIBDIR', libdir)
zrythm_launcher_config.set (
  'LD_LIBRARY_PATH_ENV',
  os_darwin ? 'DYLD_LIBRARY_PATH' : 'LD_LIBRARY_PATH')
zrythm_launcher_config.set ('FLATPAK_BUILD', flatpak_build ? '1' : '0')

# install desktop file and launcher scripts
if os_gnu or os_freebsd or os_darwin
  zrythm_launcher = configure_file (
    input: 'zrythm_launch.in',
    output: prog_name_lowercase + '_launch',
    configuration: zrythm_launcher_config,
    install: true,
    install_dir: bindir,
    install_mode: 'r-xr-xr-x')
endif

if os_gnu or os_freebsd
  zrythm_gdb = configure_file (
    input: 'zrythm_gdb.in',
    output: prog_name_lowercase + '_gdb',
    configuration: zrythm_launcher_config,
    install: true,
    install_dir: bindir,
    install_mode: 'r-xr-xr-x')

  zrythm_gdb = configure_file (
    input: 'zrythm_valgrind.in',
    output: prog_name_lowercase + '_valgrind',
    configuration: zrythm_launcher_config,
    install: true,
    install_dir: bindir,
    install_mode: 'r-xr-xr-x')

  # desktop entry
  desktop_file = configure_file (
    input: 'zrythm.desktop.in',
    output: 'zrythm.desktop.in',
    configuration: zrythm_launcher_config)
  localized_desktop_file = i18n.merge_file (
    input: desktop_file,
    output: app_id + '.desktop',
    type: 'desktop',
    po_dir: '../po',
    install: true,
    install_dir: datadir / 'applications',
    )
  desktop_utils = find_program (
    'desktop-file-validate', required: false)
  if desktop_utils.found()
    test ('Validate desktop file', desktop_utils,
      args: [localized_desktop_file]
    )
  endif

  # appdata
  # TODO generate <releases/> based on CHANGELOG.md
  appdata_config = configuration_data ({
    'HOMEPAGE_URL': 'https://www.zrythm.org/en/index.html',
    'DONATION_URL': 'https://www.zrythm.org/en/community.html',
    'BUG_REPORT_URL': 'https://todo.sr.ht/~alextee/zrythm-bug',
    'USER_MANUAL_URL': 'https://manual.zrythm.org/en/index.html',
    'PIANO_ROLL_SCREENSHOT_URL': 'https://www.zrythm.org/static/images/screenshots/Screenshot_20220110_152402.png',
    'MIXER_SCREENSHOT_URL': 'https://www.zrythm.org/static/images/screenshots/Screenshot_20220110_152001.png',
    'APP_ID': app_id,
    })
  appdata_file_in = configure_file (
    input: '@0@.appdata.xml.in'.format (app_id),
    output: '@0@.appdata.xml.in'.format (app_id),
    configuration: appdata_config)
  appdata_file = i18n.merge_file (
    input: appdata_file_in,
    output: '@0@.appdata.xml'.format (app_id),
    type: 'xml',
    po_dir: '../po',
      install: true,
      install_dir: datadir / 'metainfo',
    )
# Validate the appdata file
  appstream_util = find_program (
    'appstream-util', required: false)
  if appstream_util.found ()
    test ('validate-appdata', appstream_util,
      suite: 'data',
      args: [
        'validate-relax', '--nonet',
        appdata_file.full_path()
      ],
      depends: [
        appdata_file,
      ],
    )
  endif
endif # gnu or freebsd

if os_darwin
  zrythm_gdb = configure_file (
    input: 'zrythm_lldb.in',
    output: prog_name_lowercase + '_lldb',
    configuration: zrythm_launcher_config,
    install: true,
    install_dir: bindir,
    install_mode: 'r-xr-xr-x')
endif

subdir ('icon-themes')

# Zrythm project MIME type
install_data (
  'org.zrythm.Zrythm-mime.xml',
  install_dir: datadir / 'mime/packages')

# GtkSourceView styles
install_data (
  'gtksourceview-monokai-extended.xml',
  install_dir: sourceviewstylesdir,
  rename: 'monokai-extended-zrythm.xml')

# css themes
subdir ('css-themes')

subdir ('scripts')
subdir ('windows')
