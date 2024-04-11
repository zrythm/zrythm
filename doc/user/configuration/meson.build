# SPDX-FileCopyrightText: © 2021, 2023 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

copy_files = [
  'device-setup.rst',
  'intro.rst',
  'preferences.rst',
  'shortcuts.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
