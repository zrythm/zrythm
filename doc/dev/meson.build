# Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
  )

doxyfile_mcss = configure_file (
  output: 'Doxyfile-mcss',
  input: 'Doxyfile-mcss.in',
  configuration: cdata,
  )

extra_dev_docs = files([
  meson_src_root / 'CONTRIBUTING.md',
  'mainpage.h',
  'cyaml_schemas.h',
  'gtk_tips.md',
  'processing_cycle.h',
  'release_checklist.h',
  'weblate.h',
  ])

if dot_bin.found()
  mcss_docs_target = custom_target (
    'mcss-dev-docs',
    depend_files: [
      doxyfile,
      doxyfile_mcss,
      zrythm_srcs,
      extra_dev_docs,
      ],
    output: 'mcss-dev-docs-generated',
    command: [
      meson.current_source_dir () / 'm.css' /
      'documentation' / 'doxygen.py',
      doxyfile_mcss, '&&',
      'touch', '@OUTPUT@',
      ],
    install: false,
    build_by_default: false,
    )

  subdir ('sphinx')

  run_target (
    'gen-mcss-dev-docs',
    command: [
      'echo',
      'Developer Docs generated (m.css)',
      ],
    depends: mcss_docs_target,
    )
endif
