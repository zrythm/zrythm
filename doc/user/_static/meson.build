# SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

copy_files = [
  'favicon.ico',
  'z.svg',
  ]

foreach f : copy_files
  configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach

subdir ('css')
subdir ('img')
subdir ('js')
