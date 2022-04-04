# SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

copy_files = [
  'environment.rst',
  'files-and-directories.rst',
  'gnu-free-documentation-license.rst',
  'intro.rst',
  'menu-actions.rst',
  'meson.build',
  'troubleshooting.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
