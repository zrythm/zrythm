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

if sphinx_build.found () and sphinx_intl.found () and sass.found ()

sphinx_builddir = meson.current_build_dir () / '_build'
sphinx_build_opts = []
if get_option ('strict_sphinx_opts')
  sphinx_build_opts = [
    # Force-rebuild every time.
    '-E',
    # Fail on warnings.
    '-W',
    # Keep going when getting warnings.
    #'--keep-going',
    # Show full traceback on exception.
    '-T',
    ]
endif

# used to only create guile docs when possible,
# otherwise reuse what is already there
can_make_guile_docs = guile_dep.found () and get_option ('guile_snarf_docs_path') != ''

# Create custom targets (each API .rst file) and save
# them in guile_docs
guile_docs = []
if can_make_guile_docs
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
    texi_file = '-'.join (base_file.split ('_')) + '.texi'
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
        guild.full_path (),
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

# copy the Guile API docs to scripting/api in the
# source code (note: this edits rst source code)
copy_guile_docs_sh = configure_file (
  output: 'copy_guile_docs.sh',
  input: 'copy_guile_docs.sh',
  configuration: {
    'BASH': bash.full_path (),
    'GUILE_DOCS_SRCDIR': meson.current_build_dir (),
    'GUILE_DOCS_DESTDIR': meson.current_source_dir () / 'scripting' / 'api',
    })
copy_guile_docs_target = custom_target (
  'guile_localized_docs',
  output: 'guile_localized_docs',
  input: copy_guile_docs_sh,
  command: '@INPUT@',
  depends: [
    guile_docs,
    ])

custom_css_target = custom_target (
  'custom-css',
  input: [
    'custom_css.scss',
    ],
  output: 'custom_css_created',
  command: [
    'mkdir', '-p',
    meson.current_source_dir () / '_static/css',
    '&&',
    sass, '--sourcemap=none',
    '@INPUT@',
    meson.current_source_dir () / '_static/css/custom.css',
    '&&', 'touch', '@OUTPUT@' ]
  )
run_target (
  'manual_compile_css',
  command : [
    'echo', 'Compiled custom CSS',
    ],
  depends: custom_css_target)

src_rst_files = files ([
  'index.rst',
  ])

rtd_theme_files = files ([
  'sphinx_zrythm_theme/layout.html',
  'sphinx_zrythm_theme/search.html',
  ])

# create run targets for each lang/format combination
# and custom targets for each language for html
html_manuals = []
singlehtml_manuals = []
foreach lang, lang_str : locales
  foreach format : [
    # note: devhelp requires the devhelp package
    # from sphinxcontrib
    'devhelp', 'html', 'singlehtml', 'latex',
    'epub',
    ]

    name = format + '-manual-' + lang
    output_dir =  sphinx_builddir / lang / format
    #output_dir =  meson.current_build_dir () / lang / format
    command = [
      sphinx_build, sphinx_build_opts,
      '-D', 'language=' + lang,
      '-b', format, meson.current_source_dir (),
      output_dir, '&&', 'touch', '@OUTPUT@',
      ]

    deps = [ custom_css_target ]
    if can_make_guile_docs
      deps += copy_guile_docs_target
    endif

    run_target (
      name,
      command: command,
      depends: deps)

    if format == 'html' or format == 'singlehtml'
      html_manual = custom_target (
        name,
        depend_files: [
          'conf.py', src_rst_files,
          rtd_theme_files,
          ],
        output: name,
        command: command,
        install: get_option ('user_manual'),
        install_dir: docdir,
        build_by_default: get_option ('user_manual'),
        depends: deps,
        )
      if format == 'html'
        html_manuals += html_manual
      elif format == 'singlehtml'
        singlehtml_manuals += html_manual
      endif
    endif
  endforeach
endforeach

# Generate POT files in the build dir
#
# Note that the .pot files are not saved in git.
# They are only created temporarily and used to
# update the .po files.
manual_gettext_gen_pot = custom_target (
  'manual_gettext_gen_pot',
  output: 'manual_gettext_gen_pot',
  command: [
    sphinx_build,
    '-M', 'gettext',
    meson.current_source_dir (),
    sphinx_builddir,
    ],
  depends: can_make_guile_docs ? copy_guile_docs_target : [],
  )

sphinx_intl_update_targets = []
foreach lang, lang_str : locales
  if lang != 'en'
    # note: this updates source files
    po_file = meson.current_source_dir () / 'locale' / lang / 'LC_MESSAGES/zrythm-manual.po'
    sphinx_intl_update_target = custom_target (
      lang + '-intl-update',
      output: lang + '-intl-update',
      command: [
        # merge duplicate translations
        msguniq, '--use-first', '--output=' + po_file,
        po_file, '&&',
        # remove empty obsolete translations and
        # set the remaining as non-obsolete (so
        # they can be processed by sphinx)
        msgattrib, '--clear-obsolete', '--output=' + po_file,
        po_file, '&&',
        sphinx_intl, 'update', '-p',
        sphinx_builddir / 'gettext',
        '-l', lang,
        '-d', meson.current_source_dir () / 'locale'
        ],
      depends: manual_gettext_gen_pot,
      )
    sphinx_intl_update_targets += sphinx_intl_update_target
  endif
endforeach

# update PO in the source using the POT files in the
# build dir (note: this updates source files)
manual_gettext_update_po = custom_target (
  'manual_gettext_update_po',
  output: 'manual_gettext_update',
  command: [
    'echo', 'Updated localization',
    ],
  depends: [
    sphinx_intl_update_targets,
    manual_gettext_gen_pot,
    ],
  )

# post process localization after PO were updated
post_process_po_py = configure_file (
  output: 'post_process_po.py',
  input: 'post_process_po.py',
  configuration: {
    'PYTHON3': python3.full_path (),
    'MANUAL_SRC_DIR': meson.current_source_dir (),
    })
manual_gettext_post = custom_target (
  'manual_gettext_post',
  output: 'manual_gettext_post',
  input: post_process_po_py,
  command: [
    python3, '@INPUT@',
    ],
  depends: manual_gettext_update_po,
  )

bundle_manual_sh = configure_file (
  output: 'bundle_manual.sh',
  input: 'bundle_manual.sh',
  configuration: {
    'BASH': bash.full_path (),
    })
bundle_manual_target = custom_target (
  '_bundle_manual',
  output: 'bundled_manual.zip',
  input: [
    bundle_manual_sh,
    ],
  command: [
    '@INPUT0@', locales_str,
    meson.current_build_dir () / '_rendered',
    sphinx_builddir, '@OUTPUT@',
    '@PRIVATE_DIR@',
    ],
  depends: [
    manual_gettext_post, html_manuals,
    singlehtml_manuals,
    ],
  )

run_target (
  'manual_bundle',
  command : [
    'echo', 'Bundled user manual',
    ],
  depends: bundle_manual_target)

run_target (
  'manual_gettext',
  command : [
    'echo', 'Processed localization files',
    ],
  depends: manual_gettext_post)

run_target (
  'manual_help',
  command: [
    sphinx_build, '-M', 'help',
    meson.current_source_dir (),
    meson.current_build_dir (),
    ]
  )

endif

subdir ('locale')
