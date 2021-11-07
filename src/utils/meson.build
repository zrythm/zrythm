# Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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

util_srcs = [
  'algorithms.c',
  'arrays.c',
  'audio.c',
  'backtrace.c',
  'cairo.c',
  'chromaprint.c',
  'color.c',
  'cpu_windows.cpp',
  'curl.c',
  'datetime.c',
  'dialogs.c',
  'dictionary.c',
  'env.c',
  'error.c',
  'file.c',
  'general.c',
  'gtk.c',
  'hash.c',
  'io.c',
  'lilv.c',
  'localization.c',
  'log.c',
  'math.c',
  'mem.c',
  'object_utils.c',
  'object_pool.c',
  'objects.c',
  'resources.c',
  #'smf.c',
  'sort.c',
  'stack.c',
  'string.c',
  'strv_builder.c',
  'symap.c',
  'system.c',
  'ui.c',
  'vamp.cpp',
  'windows_errors.c',
  'yaml.c',
  ]

# optimized utils
optimized_utils_lib = static_library (
  'zrythm-optimized-utils-lib',
  sources: [
    'dsp.c',
    'mpmc_queue.c',
    'pcg_rand.c',
    ],
  dependencies: zrythm_deps,
  include_directories: all_inc,
  c_args: [
    common_cflags, strict_cflags, '-O3',
    extra_optimizations_cflags,
    extra_extra_optimizations_cflags,
    ],
  )

zrythm_srcs += files (util_srcs)

# use custom lv2apply until upstream merges URID
# support
lv2apply = executable (
  'zrythm_lv2apply',
  sources: [
    'lv2apply.c',
    'objects.c',
    'symap.c',
    ],
  dependencies : [
    sndfile_dep,
    lilv_dep,
    gtk_dep,
    libm,
    ],
  include_directories: all_inc,
  c_args: [ common_cflags ],
  link_args: common_ldflags,
  install: true,
  )
