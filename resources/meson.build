# SPDX-FileCopyrightText: © 2019-2020, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# gen resources XML
resource_files = files ([
  'gtk/help-overlay.ui',
  'gtk/menus.ui',
  ])

subdir ('ui')

zrythm_gresources_xml = custom_target (
  'zrythm-gresources-xml',
  output: 'zrythm.gresources.xml',
  input: gen_gtk_resources_xml_wrap,
  depend_files: [ resource_files, gen_gtk_resources_xml_wrap ],
  depends: appdata_file,
  command: [
    bash, '-c', '@INPUT@' + ' "$1" "$2" "$3"',
    '_ignored',
    meson.current_source_dir(),
    appdata_file.full_path(),
    meson.current_build_dir() / 'zrythm.gresources.xml'
    ])

zrythm_resources = gnome.compile_resources (
  'zrythm-resources',
  zrythm_gresources_xml,
  dependencies: appdata_file,
  source_dir: resources_dir,
  c_name: 'zrythm',
)
