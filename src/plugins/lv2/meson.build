# SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

lv2_srcs = [
  #'lv2_control.c',
  'lv2_log.c',
  'lv2_evbuf.c',
  'lv2_gtk.c',
  #'lv2_port.c',
  'lv2_state.c',
  'lv2_ui.c',
  'lv2_urid.c',
  'lv2_worker.c',
  ]

subdir('suil')

lv2_lib = static_library (
  'lv2-lib',
  sources: [
    lv2_srcs,
    ],
  dependencies: zrythm_deps,
  include_directories: all_inc,
  c_args: [ common_cflags ],
  )
