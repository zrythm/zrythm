# SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

vamp_sdk_dep = dependency (
  'vamp-sdk', static: vamp_static)

vamp_plugins_lib = static_library (
  'vamp-plugins',
  'FixedTempoEstimator.cpp',
  'BeatTrack.cpp',
  pic: true,
  link_with: [
    qm_dsp_lib,
    ],
  include_directories: [
    include_directories ('.'),
    qm_dsp_inc,
    ],
  dependencies: vamp_sdk_dep,
  )
