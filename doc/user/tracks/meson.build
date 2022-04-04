# SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

copy_files = [
  'automation.rst',
  'inspector-page.rst',
  'intro.rst',
  'track-types.rst',
  'creating-tracks.rst',
  'overview.rst',
  'track-controls.rst',
  'track-lanes.rst',
  'track-operations.rst',
  'track-visibility.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
