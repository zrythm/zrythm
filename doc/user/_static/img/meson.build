# SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

c = run_command ([
  'find', '.', '(',
  '-name', '*.png', '-o',
  '-name', '*.svg', ')',
  ])
png_files = c.stdout().strip().split('\n')

foreach f : png_files
  configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach

c = run_command ([
  'find',
  meson_src_root / 'data/icon-themes/zrythm-dark/scalable',
  '-name', '*.svg',
  ])
gui_icons = c.stdout().strip().split('\n')
foreach icon : gui_icons
  # enable to add icons
  configure_file (
    copy: true,
    input: icon,
    output: '@PLAINNAME@')
endforeach
