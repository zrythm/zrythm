# SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

if os_windows
  zrythm_rc = configure_file (
    output: 'zrythm.rc',
    input: 'zrythm.rc.in',
    configuration: {
      'ZRYTHM_MAJOR_VER': 1,
      'ZRYTHM_MINOR_VER': 0,
      'ZRYTHM_PATCH_VER': 0,
      'ZRYTHM_MICRO_VER': 0,
      'COPYRIGHT_NAME': copyright_name,
      'COPYRIGHT_YEARS': copyright_years,
      'PRODUCT_VERSION': meson.project_version(),
      })

  zrythm_ico = configure_file (
    output: 'zrythm.ico',
    input: 'zrythm.ico',
    copy: true)

  winmod = import('windows')
  win_res = winmod.compile_resources (
    zrythm_rc,
    depend_files: 'zrythm.ico')
endif
