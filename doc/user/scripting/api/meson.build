# SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# used to only create guile docs when possible,
# otherwise reuse what is already there
can_make_guile_docs = guile_dep.found () and get_option ('guile_snarf_docs_path') != '' and texi2html.found () and pandoc.found ()

# Create custom targets (each API .rst file) and save
# them in guile_docs
guile_docs = []
if can_make_guile_docs
  gen_texi_docs_sh = find_program (
    'guile_gen_texi_docs.sh')
  format_rst = find_program ('format_rst.sh')
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
      meson_src_root)[1].split('/'))
    base_file = base_file.split('src guile ')[1]
    rst_tmp_file = base_file + '.rst.tmp'
    rst_file = base_file + '.rst'
    texi_file = '-'.join (base_file.split ('_')) + '.texi'
    texi_doc = configure_file (
      output: texi_file,
      command: [
        gen_texi_docs_sh,
        get_option ('guile_snarf_docs_path'),
        guile_dep.name(), snarfable_src,
        texi_file, texi_file + '.p',
        guild.full_path (),
        '-I' + meson_src_root / 'inc',
        ],
      )
    rst_tmp_doc = configure_file (
      output: rst_tmp_file,
      input: texi_doc,
      command: [
        # FIXME this should be a shell script, not
        # guile script
        guile_gen_docs_scm,
        guile_dep.name(),
        meson.current_build_dir () / texi_file,
        '@OUTPUT@',
        meson.current_build_dir () / rst_tmp_file + '.p',
        ],
      )
    rst_doc = configure_file (
      output: rst_file,
      input: rst_tmp_doc,
      command: [
        format_rst,
        meson.current_build_dir () / rst_tmp_file,
        '@OUTPUT@',
        ],
      )
    rst_files += rst_doc
    guile_docs += rst_doc
  endforeach
# else if can't make guile docs, copy existing ones
else
  c = run_command (['find', '.', '-name', '*.rst'])
  existing_api_docs = c.stdout().strip().split('\n')
  foreach existing_api_doc : existing_api_docs
    rst_files += configure_file (
      copy: true,
      input: existing_api_doc,
      output: '@PLAINNAME@')
  endforeach
endif

# copy the Guile API docs to /api in the
# source code (note: this edits rst source code)
copy_guile_docs_sh = configure_file (
  output: 'copy_guile_docs.sh',
  input: 'copy_guile_docs.sh',
  configuration: {
    'BASH': bash.full_path (),
    'GUILE_DOCS_SRCDIR': meson.current_build_dir (),
    'GUILE_DOCS_DESTDIR': meson.current_source_dir (),
    })
copy_guile_docs_target = custom_target (
  'guile_localized_docs',
  output: 'guile_localized_docs',
  command: copy_guile_docs_sh,
  depend_files: [
    guile_docs,
    ])

copy_files = [
  'intro.rst',
  ]

foreach f : copy_files
  rst_files += configure_file (
    copy: true,
    input: f,
    output: '@PLAINNAME@')
endforeach
