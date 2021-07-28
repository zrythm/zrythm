# Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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

noisegate_uri = 'https://lv2.zrythm.org/VeJaPlugins/NoiseGate'

noisegate_cflags = [
  '-Wall', '-Wextra', '-pipe',
  '-Wno-unused-parameter', '-O3', '-g',
  '-DPLUGIN_URI="' + noisegate_uri + '"',
  '-DNDEBUG', '-ffast-math',
  '-fvisibility=hidden',
  '-fvisibility-inlines-hidden' ]

if not os_windows
  noisegate_cflags += [ '-fPIC', '-DPIC' ]
endif

noisegate_linkargs = []
if os_darwin
  noisegate_cflags += [ '-dynamiclib' ]
  noisegate_linkargs += [
    '-Wl,-dead_strip', '-Wl,-dead_strip_dylibs',
    '-Wl,--no-undefined' ]
else
  noisegate_cflags += [ '-shared' ]
  noisegate_linkargs += [
    '-Wl,-O1', '-Wl,--as-needed', '-Wl,--strip-all' ]
endif

noisegate_lv2_install_dir = zrythmdatadir / 'lv2/noisegate.lv2'
noisegate_so = shared_library (
  'noisegate',
  'circular_buffer.c',
  'noisegate.c',
  'gate_core.c',
  dependencies: libm,
  name_prefix: '',
  gnu_symbol_visibility: 'hidden',
  c_args: noisegate_cflags,
  link_args: noisegate_linkargs,
  install: true,
  install_dir: noisegate_lv2_install_dir,
  )

noisegate_lv2_config = configuration_data ()
noisegate_lv2_config.set (
  'PLUGIN_URI', noisegate_uri)

noisegate_lv2_manifest_ttl = configure_file (
  input: 'noisegate.lv2/manifest.ttl',
  output: 'manifest.ttl',
  configuration: noisegate_lv2_config,
  install: true,
  install_dir: noisegate_lv2_install_dir)
noisegate_lv2_ttl = configure_file (
  input: 'noisegate.lv2/noisegate.ttl',
  output: 'noisegate.ttl',
  configuration: noisegate_lv2_config,
  install: true,
  install_dir: noisegate_lv2_install_dir)
