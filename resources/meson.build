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

gen_gtk_resources_filename = 'gen-gtk-resources-xml.scm'
gen_gtk_resources = configure_file (
  output: gen_gtk_resources_filename,
  input: gen_gtk_resources_filename,
  configuration: scripts_conf,
  )
# wrap is needed on windows mingw
gen_gtk_resources_wrap = configure_file (
  output: 'gen_gtk_resources_xml_wrap.sh',
  input: 'gen_gtk_resources_xml_wrap.sh',
  configuration: scripts_conf,
  )
zrythm_gresources_xml = configure_file (
  output: 'zrythm.gresources.xml',
  input: gen_gtk_resources,
  command: [
    gen_gtk_resources_wrap,
    meson.current_source_dir(),
    '@OUTPUT@'
    ])
zrythm_resources = gnome.compile_resources (
  'zrythm-resources',
  zrythm_gresources_xml,
  source_dir: resources_dir,
  c_name: 'zrythm',
)

# install app icon
install_data (
  join_paths ('icons', 'zrythm', 'zrythm.svg'),
  install_dir:
    join_paths (
      datadir, 'icons', 'hicolor', 'scalable',
      'apps'))
