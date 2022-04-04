# SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

copy_files = [
  'inspector-page.rst',
  'intro.rst',
  'plugin-browser.rst',
  'plugin-info.rst',
  'plugin-window.rst',
  'scanning.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
