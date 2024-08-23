# SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
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
  'env.cpp',
  'error.cpp',
  'exceptions.cpp',
  'file.cpp',
  'gtk.cpp',
  'hash.cpp',
  'io.cpp',
  'json.cpp',
  'localization.cpp',
  'logger.cpp',
  'math.cpp',
  'mem.cpp',
  'objects.cpp',
  'pango.cpp',
  'progress_info.cpp',
  'resampler.cpp',
  'resources.cpp',
  'rt_thread_id.cpp',
  'sort.cpp',
  'string.cpp',
  'symap.cpp',
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
    glibmm_dep,
    giomm_dep,
    libm,
    ],
  include_directories: all_inc,
  c_args: [ common_cflags ],
  link_args: common_ldflags,
  install: false,
  )
