# SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

util_srcs = [
  'algorithms.c',
  'arrays.c',
  'audio.c',
  'backtrace.c',
  'cairo.c',
  'chromaprint.c',
  'color.c',
  'compression.c',
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
  'localization.c',
  'log.c',
  'math.c',
  'mem.c',
  'object_pool.c',
  'objects.c',
  'pango.c',
  'progress_info.c',
  'resampler.c',
  'resources.c',
  'sort.c',
  'stack.c',
  'string.c',
  'symap.c',
  'system.c',
  'ui.c',
  'vamp.cpp',
  'windows.c',
  'windows_errors.c',
  'yaml.c',
  ]

# optimized utils
optimized_utils_lib = static_library (
  'zrythm-optimized-utils-lib',
  sources: [
    'dsp.c',
    'midi.c',
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

# use custom lv2apply because upstream doesn't have URID support
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
  install: false,
  )
