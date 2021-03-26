# Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
#
# This file is part of Zrythm
#
# Zrythm is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, version 3.
#
# Zrythm is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.

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
