# SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

if (not help2man.found ()) or os_windows or not get_option ('manpage')
  subdir_done ()
endif

manpage_h2m = configure_file (
  output: 'zrythm.h2m',
  input: 'zrythm.h2m.in',
  configuration: {
    'COPYRIGHT_NAME': copyright_name,
    'COPYRIGHT_YEARS': copyright_years,
    }
  )

manpage = custom_target (
  'manpage',
  input: manpage_h2m,
  output: 'zrythm.1',
  depends: zrythm_exe,
  command: [
    help2man,
    '-s',
    '1',
    '-N',
    '-i',
    '@INPUT@',
    '-o',
    '@OUTPUT@',
    '-n',
    'Zrythm',
    '--version-string=Zrythm ' + meson.project_version (),
    zrythm_exe,
    ],
  install: get_option ('manpage'),
  install_dir: mandir,
  build_by_default: get_option ('manpage'),
  )

# gen completions
if os_gnu
  manpage_completions_run_sh = find_program (
    meson_src_root / 'ext/sh-manpage-completions/run.sh')

  gen_completions_py = 'gen_completions.py'

  # fish
  fish_completion = custom_target (
    'fish-completion',
    output: 'zrythm.fish',
    input: [manpage, gen_completions_py],
    command: [
      python3, '@INPUT1@', manpage.full_path (), 'fish',
      '@OUTPUT@', manpage_completions_run_sh.full_path(),
      ],
    install: get_option ('completions'),
    install_dir: datadir / 'fish/vendor_completions.d',
    build_by_default: get_option ('completions'))

  # bash
  bash_completion = custom_target (
    'bash-completion',
    output: 'zrythm',
    input: [manpage, gen_completions_py],
    depends: fish_completion,
    command: [
      python3, '@INPUT1@', manpage.full_path (), 'bash',
      '@OUTPUT@', manpage_completions_run_sh.full_path(),
      ],
    install: get_option ('completions'),
    install_dir: datadir / 'bash-completion/completions',
    build_by_default: get_option ('completions'))

  # skip zsh completion for now - causes errors
  # with old bash versions
  if false
    zsh_completion = custom_target (
      'zsh-completion',
      output: '_zrythm',
      input: [manpage, gen_completions_py],
      depends: fish_completion,
      command: [
        python3, '@INPUT1@', manpage.full_path (), 'zsh',
        '@OUTPUT@', manpage_completions_run_sh.full_path(),
        ],
      install: get_option ('completions'),
      install_dir: datadir / 'zsh/site-functions',
      build_by_default: get_option ('completions'))
  endif
endif

run_target (
  'build-manpage',
  command: [
    'echo',
    'Manpage built',
    ],
  depends: manpage,
  )
