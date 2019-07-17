# Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
#
# This file is part of Zrythm
#
# Zrythm is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Zrythm is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.

doxyfile = configure_file (
  output: 'Doxyfile.cfg',
  input: 'Doxyfile.cfg.in',
  configuration: cdata,
  install: false,
  )

doxyfile_mcss = configure_file (
  output: 'Doxyfile-mcss',
  input: 'Doxyfile-mcss.in',
  configuration: cdata,
  install: false,
  )

html_target = custom_target (
  'html-docs',
  depend_files: [
    doxyfile,
    doxyfile_mcss,
    zrythm_srcs,
    'mainpage.h',
    'cyaml_schemas.h',
    'processing_cycle.h',
    'release_checklist.h',
    'weblate.h',
    ],
  output: 'html',
  command: [
    join_paths (meson.current_source_dir (), 'm.css', 'documentation', 'doxygen.py'),
    doxyfile_mcss,
    ],
  install: false,
  )

run_target (
  'docs',
  command: [
    'echo',
    'Developer Docs generated',
    ],
  depends: html_target,
  )

run_target (
  'clean_docs',
  command: [
    'rm',
    '-rf',
    'html',
    ],
  )
