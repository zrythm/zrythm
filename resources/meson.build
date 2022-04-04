# SPDX-FileCopyrightText: © 2019-2020 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# gen resources XML
resource_files = files ([
  'gtk/help-overlay.ui',
  'gtk/menus.ui',
  ])

subdir ('ui')

zrythm_gresources_xml = configure_file (
  output: 'zrythm.gresources.xml',
  input: [
    gen_gtk_resources_xml_wrap,
    resource_files,
    ],
  command: [
    '@INPUT0@',
    meson.current_source_dir(),
    '@OUTPUT@'
    ])

zrythm_resources = gnome.compile_resources (
  'zrythm-resources',
  zrythm_gresources_xml,
  source_dir: resources_dir,
  c_name: 'zrythm',
)
