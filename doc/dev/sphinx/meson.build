# Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

if sphinx_build.found ()
  # note: needs exhale to be inside the ext dir
  sphinx_dev_target = custom_target (
    'sphinx-dev-docs',
    depend_files: [
      doxyfile,
      zrythm_srcs,
      extra_dev_docs,
      ],
    input: [
      'source/conf.py',
      'source/index.rst',
      ],
    output: 'sphinx-dev-docs-built',
    command: [
      sphinx_build, '-b', 'html',
      '-D', 'breathe_projects.Zrythm=' + meson.current_build_dir () / '..' / 'xml/',
      '-j', '4',
      meson.current_source_dir () / 'source',
      meson.current_build_dir () / 'build', '&&',
      'touch', '@OUTPUT@',
      ],
    install: false,
    build_by_default: false,
    depends: mcss_docs_target,
    )

  run_target (
    'gen-sphinx-dev-docs',
    command: [
      'echo',
      'Developer Docs generated (sphinx)',
      ],
    depends: sphinx_dev_target,
    )
endif
