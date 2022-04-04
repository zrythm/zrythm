# SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

copy_files = [
  'common-operations.rst',
  'edit-tools.rst',
  'event-viewers.rst',
  'intro.rst',
  'overview.rst',
  'quantization.rst',
  'snapping-grid-options.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach

subdir ('clip-editors')
subdir ('timeline')
