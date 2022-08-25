# SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

copy_files = [
  'bottom-panel.rst',
  'intro.rst',
  'left-panel.rst',
  'main-panel.rst',
  'main-toolbar.rst',
  'overview.rst',
  'right-panel.rst',
  'transport-bar.rst',
  ]

foreach f : copy_files
  configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
