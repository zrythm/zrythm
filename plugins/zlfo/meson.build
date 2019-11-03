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

zlfo_cdata = configuration_data ()
zlfo_cdata.set (
  'PROJECT_URI',
  'https://lv2.zrythm.org/plugins')
zlfo_cdata.set (
  'LFO_URI',
  zlfo_cdata.get ('PROJECT_URI') + '/ZLFO')
zlfo_cdata.set (
  'LFO_UI_URI',
  zlfo_cdata.get ('LFO_URI') + '#UI')
zlfo_cdata.set (
  'LFO_DSP_BINARY',
  'zlfo_dsp.so')
zlfo_cdata.set (
  'LFO_UI_BINARY',
  'zlfo_ui.so')
zlfo_cdata.set (
  'LFO_TTL',
  'zlfo.ttl')

zlfo_cdata_quoted = configuration_data ()
zlfo_cdata_quoted.set_quoted (
  'PROJECT_URI',
  zlfo_cdata.get ('PROJECT_URI'))
zlfo_cdata_quoted.set_quoted (
  'LFO_URI',
  zlfo_cdata.get ('LFO_URI'))
zlfo_cdata_quoted.set_quoted (
  'LFO_UI_URI',
  zlfo_cdata.get ('LFO_UI_URI'))

# create config.h
zlfo_config_h = configure_file (
  output: 'config.h',
  configuration: zlfo_cdata_quoted,
  )
zlfo_config_h_dep = declare_dependency (
  sources: zlfo_config_h,
  )

zlfo_deps = [
  zlfo_config_h_dep,
  plugin_deps,
  ]

zlfo_dsp_lib = shared_library (
  'zlfo_dsp',
  name_prefix: '',
  sources: [
    'zlfo.c',
    ],
  dependencies: zlfo_deps,
  include_directories: plugins_common_inc,
  c_args: [ plugin_cflags, ],
  install: true,
  install_dir: lv2zpluginsdir,
  )

zlfo_ui_lib = shared_library (
  'zlfo_ui',
  name_prefix: '',
  sources: [
    'zlfo_ui.c',
    ],
  dependencies: zlfo_deps,
  include_directories: plugins_common_inc,
  c_args: [ plugin_cflags, ],
  link_with: [
    ztoolkit_lib,
    ],
  install: true,
  install_dir: lv2zpluginsdir,
  )

# create and install ttls
manifest_ttl = configure_file (
  input: 'manifest.ttl.in',
  output: 'manifest.ttl',
  configuration: zlfo_cdata,
  )
zlfo_ttl = configure_file (
  input: 'zlfo.ttl.in',
  output: 'zlfo.ttl',
  configuration: zlfo_cdata,
  )
install_data (
  manifest_ttl,
  install_dir: lv2zpluginsdir,
  )
install_data (
  zlfo_ttl,
  install_dir: lv2zpluginsdir,
  )

# remember build dir
zlfo_build_dir = meson.current_build_dir ()
