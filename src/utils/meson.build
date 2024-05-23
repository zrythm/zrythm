# SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

util_srcs = [
  'algorithms.cpp',
  'arrays.cpp',
  'audio.cpp',
  'backtrace.cpp',
  'cairo.cpp',
  'chromaprint.cpp',
  'color.cpp',
  'compression.cpp',
  'cpu_windows.cpp',
  'curl.cpp',
  'datetime.cpp',
  'dialogs.cpp',
  'dictionary.cpp',
  'env.cpp',
  'error.cpp',
  'file.cpp',
  'general.cpp',
  'gtk.cpp',
  'hash.cpp',
  'io.cpp',
  'localization.cpp',
  'log.cpp',
  'math.cpp',
  'mem.cpp',
  'object_pool.cpp',
  'objects.cpp',
  'pango.cpp',
  'progress_info.cpp',
  'resampler.cpp',
  'resources.cpp',
  'sort.cpp',
  'stack.cpp',
  'string.cpp',
  'symap.cpp',
  'system.cpp',
  'ui.cpp',
  'vamp.cpp',
  'windows.cpp',
  'windows_errors.cpp',
  'yaml.cpp',
  ]

# optimized utils
optimized_utils_lib = static_library (
  'zrythm-optimized-utils-lib',
  sources: [
    'dsp.cpp',
    'midi.cpp',
    'mpmc_queue.cpp',
    'pcg_rand.cpp',
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
    'lv2apply.cpp',
    'objects.cpp',
    'symap.cpp',
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
