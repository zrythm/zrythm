# SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
  'PREFIX', prefix)
zrythm_launcher_config.set (
  'DATADIR', datadir)
zrythm_launcher_config.set (
  'SCHEMAS_DIR', schemasdir)
zrythm_launcher_config.set ('BIN_DIR', bindir)
zrythm_launcher_config.set (
  'DESKTOP_ENTRY_NAME',
  installer_ver ? 'Zrythm beta' : prog_name)
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
zrythm_launcher_config.set ('IS_INSTALLER_VER', get_option ('installer_ver') ? '1' : '0')

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
    po_dir: meson_build_root / 'po',
    install: true,
    install_dir: datadir / 'applications',
    )
  run_target (
    'gen-desktop-file',
    command: [ 'echo', 'Generated desktop file' ],
    depends: localized_desktop_file)
  desktop_utils = find_program (
    'desktop-file-validate', required: false)
  if desktop_utils.found()
    test ('Validate desktop file', desktop_utils,
      args: [localized_desktop_file]
    )
  endif
endif # gnu or freebsd

# appdata
gen_appdata_xml_py_filename = 'gen-appdata-xml.py'
gen_appdata_xml_py = configure_file (
  output: gen_appdata_xml_py_filename,
  input: gen_appdata_xml_py_filename,
  configuration: scripts_conf
  )
appdata_file_in_unformatted = custom_target (
  'appdata-xml-in-tmp',
  input: gen_appdata_xml_py,
  output: '@0@.appdata.xml.in.in'.format (app_id),
  command: [
    python3, '@INPUT@', meson_build_root / '@OUTPUT@', app_id
    ])
appdata_file_in = custom_target (
  'appdata-xml-in',
  input: appdata_file_in_unformatted,
  output: '@0@.appdata.xml.in'.format (app_id),
  command: [
    xmllint, '--format', '--output', '@OUTPUT@', '@INPUT@',
    ])

if os_windows
  # we have an issue similar to:
  # https://gitlab.gnome.org/GNOME/gimp/-/issues/10742
  # except that appstream and gettext-tools are already installed
  # just skip merging translations for now and use the file as-is
  appdata_file = custom_target (
    'appdata-xml',
    input: appdata_file_in,
    output: '@0@.appdata.xml'.format (app_id),
    command: [
      xmllint, '--format', '--output', '@OUTPUT@', '@INPUT@',
      ])
else
  appdata_file = i18n.merge_file (
    input: appdata_file_in,
    output: '@0@.appdata.xml'.format (app_id),
    type: 'xml',
    po_dir: meson_build_root / 'po',
    install: true,
    install_dir: datadir / 'metainfo',
    )
endif

run_target (
  'gen-appdata-xml',
  command: [ 'echo', 'Generated appdata.xml' ],
  depends: appdata_file)
# Validate the appdata file
appstream_util = find_program (
  'appstream-util', required: false)
if appstream_util.found ()
  test ('appstream-util-validate-appdata',
    appstream_util,
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
appstreamcli = find_program (
  'appstreamcli', required: false)
if appstreamcli.found ()
  test ('appstream-cli-validate-appdata',
    appstreamcli,
    suite: 'data',
    args: [
      'validate',
      appdata_file.full_path()
    ],
    depends: [
      appdata_file,
    ],
  )
endif

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

# templates
install_emptydir(zrythmdatadir / 'templates')

# css themes
subdir ('css-themes')

subdir ('plugins')
subdir ('windows')
