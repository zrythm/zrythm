# SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

data_script_filenames = [
  # FIXME test fails
  #'create-geonkick-with-fx-track.scm',
  'create-midi-track-with-notes.scm',
  'hello-world.scm',
  'print-all-tracks.scm',
  ]

data_scripts = files (data_script_filenames)

foreach script : data_scripts
  install_data (
    script,
    install_dir: scriptsdir)
endforeach
