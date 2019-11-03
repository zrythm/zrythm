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

plugin_cdata = configuration_data ()
plugin_cdata.set (
  '_PROJECT_URI',
  'https://lv2.zrythm.org/plugins')
plugin_cdata.set (
  '_LFO_URI',
  plugin_cdata.get ('_PROJECT_URI') + '/ZLFO')
plugin_cdata.set (
  '_LFO_UI_URI',
  plugin_cdata.get ('_LFO_URI') + '#UI')
plugin_cdata.set_quoted (
  'PROJECT_URI',
  plugin_cdata.get ('_PROJECT_URI'))
plugin_cdata.set_quoted (
  'LFO_URI',
  plugin_cdata.get ('_LFO_URI'))
plugin_cdata.set_quoted (
  'LFO_UI_URI',
  plugin_cdata.get ('_LFO_UI_URI'))

plugins_root_inc = include_directories ('.')
plugins_src_inc = include_directories ('src')
pugl_inc = include_directories ('pugl')
plugins_all_inc = [
  plugins_root_inc,
  plugins_src_inc,
  pugl_inc,
  ]

cairo_dep = dependency('cairo', version: '>=1.0.0')
plugin_deps = [
  lv2_dep,
  fftw3_dep,
  dependency('x11'),
  cairo_dep,
  libm,
]

# create config.h
plugin_config_h = configure_file (
  output: 'config.h',
  configuration: plugin_cdata,
  )
plugin_config_h_dep = declare_dependency (
  sources: plugin_config_h,
  )
plugin_deps += plugin_config_h_dep

subdir ('pugl')
subdir ('ztoolkit')
subdir ('src')

# create and install ttls
manifest_ttl = configure_file (
  input: 'manifest.ttl.in',
  output: 'manifest.ttl',
  configuration: plugin_cdata,
  )
zlfo_ttl = configure_file (
  input: 'ZLFO.ttl.in',
  output: 'ZLFO.ttl',
  configuration: plugin_cdata,
  )
install_data (
  manifest_ttl,
  install_dir: lv2zpluginsdir,
  )
install_data (
  zlfo_ttl,
  install_dir: lv2zpluginsdir,
  )
