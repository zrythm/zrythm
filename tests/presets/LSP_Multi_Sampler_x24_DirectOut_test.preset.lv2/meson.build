# SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

configure_file (
  copy: true,
  input: 'manifest.ttl',
  output: '@PLAINNAME@')
configure_file (
  copy: true,
  input: 'test2.ttl',
  output: '@PLAINNAME@')
if rubberband.found ()
  configure_file (
    copy: true,
    input: test_wav2,
    output: 'SLSE Crash Cymbal - 010.wav')
endif
