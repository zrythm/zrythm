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

if (sphinx_build.found())

foreach lang : locales
  html_manual = custom_target (
    'html-manual-' + lang,
    depend_files: [
      'conf.py',
      ],
    output: lang,
    command: [
      sphinx_build,
      '-D',
      'language=' + lang,
      '-b',
      'html',
      meson.current_source_dir (),
      '@OUTPUT@',
      ],
    install: false,
    build_by_default: get_option ('user_manual'),
    )

  if (get_option ('user_manual'))
    install_subdir (
      join_paths (
        meson.current_build_dir (),
        lang),
      strip_directory: true,
      install_dir:
        join_paths (docdir, lang))
  endif
endforeach

if (get_option ('user_manual'))
  install_data (
    'README.md',
    install_dir: docdir)
  install_data (
    'LICENSE',
    install_dir: docdir)
endif

run_target (
  'bundle_manual',
  command: [
    'echo',
    'Bundled manual generated',
    ],
  depends: html_dev_target,
  )

endif
