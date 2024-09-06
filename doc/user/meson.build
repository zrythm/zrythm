# SPDX-FileCopyrightText: © 2019-2020 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

if not get_option('user_manual')
  subdir_done ()
endif

if (not sphinx_build.found ()) or (not sphinx_intl.found ()) or (not sass.found ())
  warning ('sphinx-build/sphinx-intl/sass not found - user manual cannot be built')
  subdir_done ()
endif

msgattrib = find_program ('msgattrib')
msguniq = find_program ('msguniq')
msggrep = find_program ('msggrep')

sphinx_builddir = meson.current_build_dir () / '_build'
sphinx_build_opts = []
if get_option ('strict_sphinx_opts')
    # Fail on warnings.
    # Keep going when getting warnings.
    #'--keep-going',
    # Show full traceback on exception.
  sphinx_build_opts = [ '-E', '-W', '-T' ]
endif

html_templates = files ([
  '_templates/page.html',
  ])

run_sphinx_build = configure_file (
  output: 'run_sphinx_build.sh',
  input: 'run_sphinx_build.sh',
  configuration: {
    'BASH': bash.full_path (),
    })

subdir ('locale')

index_rst = configure_file (
  input: 'index.rst',
  output: '@PLAINNAME@',
  configuration: {
    'COPYRIGHT_NAME': copyright_name,
    'COPYRIGHT_YEARS': copyright_years,
    })
rst_files = [ index_rst ]

subdir ('_static')
subdir ('appendix')
subdir ('chords-and-scales')
subdir ('configuration')
subdir ('contributing')
subdir ('credits')
subdir ('editing')
subdir ('exporting')
subdir ('getting-started')
subdir ('glossary')
subdir ('invoking-from-command-line')
subdir ('mixing')
subdir ('modulators')
subdir ('playback-and-recording')
subdir ('plugins-files')
subdir ('projects')
subdir ('routing')
subdir ('scripting')
subdir ('theming')
subdir ('tracks')
subdir ('zrythm-interface')

conf_py = configure_file (
  output: 'conf.py',
  input: 'conf.py.in',
  configuration: {
    'PROGRAM_NAME': prog_name,
    'VERSION': meson.project_version (),
    'COPYRIGHT_NAME': copyright_name,
    'COPYRIGHT_YEARS': copyright_years,
    'MESON_DOC_USER_SRC_DIR': meson.current_source_dir (),
    'MESON_DOC_USER_BUILD_DIR': meson.current_build_dir (),
    'ISSUE_TRACKER_URL': issue_tracker_url,
    })

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
    command = [
      run_sphinx_build,
      sphinx_build.full_path(),
      ' '.join (sphinx_build_opts),
      lang, format, meson.current_build_dir (),
      output_dir,
      ]

    deps = [
      custom_css_target,
      copy_locales_targets,
      ]

    run_target (
      name,
      command: command,
      depends: deps)

    if format == 'html' or format == 'singlehtml'
      html_manual = custom_target (
        name,
        depend_files: [
          conf_py, rst_files,
          html_templates,
          ],
        output: name,
        command: [ command, '@OUTPUT@', ],
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

# link check
link_check_target = custom_target (
  'sphinx_linkcheck_target',
  output: 'linkcheck',
  command: [
    sphinx_build, '-b', 'linkcheck',
    sphinx_build_opts,
    meson.current_build_dir (),
    sphinx_builddir / '@OUTPUT@',
    ],
  depends: html_manuals,
  install: false,
  build_by_default: get_option ('user_manual'),
  )

run_target (
  'sphinx_linkcheck',
  command: [ 'echo', 'done' ],
  depends: link_check_target)

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
    meson.current_build_dir (),
    sphinx_builddir,
    ],
  depend_files: rst_files,
  )

sphinx_intl_update_targets = []
foreach lang, lang_str : locales
  if lang != 'en'
    # note: this updates source files
    po_file = meson.current_source_dir () / 'locale' / lang / 'LC_MESSAGES/zrythm-manual.po'
    pre_command = []
    if fs.is_file(po_file)
      pre_command = [
        # merge duplicate translations
        msguniq, '--use-first', '--output=' + po_file,
        po_file, '&&',
        # remove empty obsolete translations and
        # set the remaining as non-obsolete (so
        # they can be processed by sphinx)
        msgattrib, '--clear-obsolete', '--output=' + po_file,
        po_file, '&&',
        ]
    endif
    sphinx_intl_update_target = custom_target (
      lang + '-intl-update',
      output: lang + '-intl-update',
      command: [
        pre_command, sphinx_intl, 'update', '-p',
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
    'MSGGREP': msggrep.full_path (),
    })
manual_gettext_post = custom_target (
  'manual_gettext_post',
  output: 'manual_gettext_post',
  input: post_process_po_py,
  command: [
    python3, '@INPUT@',
    ],
  depends: manual_gettext_update_po,
  console: true,
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
    bundle_manual_sh, locales_str,
    meson.current_source_dir (),
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
