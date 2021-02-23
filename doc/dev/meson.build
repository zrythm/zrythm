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

doc_cdata = configuration_data ()
doc_cdata.merge_from (cdata)
doc_cdata.set (
  'DOX_OUTPUT_DIR',
  meson.current_build_dir ())
doc_cdata.set (
  'DOC_RELEASE',
  meson.project_version ())

doxyfile = configure_file (
  output: 'Doxyfile.cfg',
  input: 'Doxyfile.cfg.in',
  configuration: doc_cdata,
  )

doxyfile_mcss = configure_file (
  output: 'Doxyfile-mcss',
  input: 'Doxyfile-mcss.in',
  configuration: doc_cdata,
  )

extra_dev_docs = files([
  meson.source_root() / 'CONTRIBUTING.md',
  'mainpage.h',
  'cyaml_schemas.h',
  'gtk_tips.md',
  'processing_cycle.h',
  'release_checklist.h',
  'weblate.h',
  ])

c_rst_files = files(
  'index.rst',
  #'overview.rst',
  )
foreach f : c_rst_files
  configure_file (copy: true, input: f, output: '@PLAINNAME@')
endforeach

subdir ('xml')

conf_py = configure_file(configuration: doc_cdata,
                         input: 'conf.py.in',
                         output: 'conf.py')

c_rst = custom_target(
  'c-api-rest',
  command: [
    dox_to_sphinx, '-f', '@INPUT0@',
    meson.current_build_dir()
    ],
  input: [c_index_xml],
  output: 'zrythm.rst')

sphinx_docs_target = custom_target(
  'html-documentation2',
  command: [
    sphinx_build, '-M', 'html',
    meson.current_build_dir(),
    meson.current_build_dir(),
    '-E', '-q', '-t', 'html'],
  input: [c_rst_files, c_rst, c_index_xml],
  output: 'html',
  install: false)

run_target (
  'gen-sphinx-docs',
  command: [
    'echo',
    'Developer Docs generated (sphinx)',
    ],
  depends: sphinx_docs_target,
  )

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
