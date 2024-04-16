# SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

copy_files = [
  'audio-editor.rst',
  'automation-editor.rst',
  'chord-editor.rst',
  'common-operations.rst',
  'edit-tools.rst',
  'editor-toolbars.rst',
  'intro.rst',
  'piano-roll.rst',
  'timeline.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
