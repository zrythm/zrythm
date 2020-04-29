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

if (sphinx_build.found())

# note: this is not used at the moment.
# `ninja -C build bundle_manual` is used instead
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
endif

# generate guile docs
guile_docs = []
if get_option ('enable_guile') and get_option ('guile_snarf_docs_path') != ''
  foreach snarfable_src : guile_snarfable_srcs
    split_by_dot = snarfable_src.split('.')
    without_ext = ''
    foreach sp : split_by_dot
      if sp == split_by_dot[split_by_dot.length() - 2]
        without_ext += sp
      elif sp != split_by_dot[split_by_dot.length() - 1]
        without_ext += sp + '.'
      endif
    endforeach
    base_file = ' '.join(without_ext.split (
      meson.source_root ())[1].split('/'))
    base_file = base_file.split('src guile ')[1]
    rst_file = base_file + '.rst'
    texi_file = base_file + '.texi'
    texi_doc = custom_target (
      texi_file,
      output: texi_file,
      input: [
        snarfable_src,
        meson.source_root () / 'scripts' / 'guile_gen_texi_docs.sh',
        ],
      command: [
        '@INPUT1@',
        get_option ('guile_snarf_docs_path'),
        guile_dep.name(), '@INPUT0@',
        meson.build_root () / '@OUTPUT@',
        meson.build_root () / '@PRIVATE_DIR@',
        guild.path (),
        '-I' + meson.source_root () / 'inc',
        ],
      )
    rst_doc = custom_target (
      rst_file,
      output: rst_file,
      input: [
        texi_doc,
        meson.build_root () / 'scripts' / 'guile-gen-docs.scm',
        ],
      command: [
        '@INPUT1@',
        guile_dep.name(),
        meson.build_root () / '@INPUT0@',
        meson.build_root () / '@OUTPUT@',
        meson.build_root () / '@PRIVATE_DIR@',
        ],
      )
    guile_docs += rst_doc
  endforeach
endif

# TODO convert the makefile to meson commands
run_target (
  'bundle_manual',
  command: [
    'make', 'update-guile-docs', 'gettext',
    'bundle',
    'BUILDDIR=' +
      meson.current_build_dir () / '_build',
    'RENDERDIR=' +
      meson.current_build_dir () / '_rendered',
    'GUILE_DOCS_SRCDIR="' +
      meson.current_build_dir () + '"',
    'GUILE_DOCS_DESTDIR="' +
      meson.current_source_dir () / 'scripting' / 'api' +
      '"',
    ],
  depends: guile_docs,
  )

run_target (
  'manual_gettext',
  command: [ 'make', 'gettext', ],
  )

endif
