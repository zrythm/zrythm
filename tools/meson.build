# SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

if jq.found () and get_option ('want_clang_tidy')

  # get files from compile_commands.json
  compile_commands_files = run_command (
    jq, '--raw-output', '.[].file',
    meson_build_root / 'compile_commands.json').stdout ().strip ().split ('\n')

  ignore_paths = [
    '../src/plugins/lv2',
    '../subprojects',
    '../ext',
    ]
  ignore_commands = []
  foreach ignore_path : ignore_paths
    ignore_commands += [ '-i' + ignore_path ]
  endforeach

  if clang_tidy.found ()
    run_clang_tidy = find_program (
      'run_clang_tidy.sh')
  endif

  # Go through each file and generate a custom target.
  #
  # The full list of other files is passed as an
  # ignore list because we can't pass a single file
  # to cppcheck when using the --project option, and
  # we want to use the --project option because it
  # contains the build commands/defines/includes.
  #
  # The targets are split so that we can allow ninja
  # to use concurrency.
  cppcheck_targets = []
  clang_tidy_targets = []
  processed_files = []
  foreach file : compile_commands_files
    skip = false
    if file in processed_files
      skip = true
    endif
    foreach ignore_path : ignore_paths
      if file.startswith (ignore_path)
        skip = true
        break
      endif
    endforeach
    if skip
      continue
    endif

    if cppcheck.found ()
      name = file.underscorify () + '.cppcheck'
      other_files = []
      foreach f : compile_commands_files
        if f != file
          other_files += '-i' + f
        endif
      endforeach
      cppcheck_targets += custom_target (
        name,
        output: name,
        depends: [ config_h_vcs ],
        command: [
          cppcheck,
          '--project=' + meson_build_root / 'compile_commands.json',
          '--include=' + meson_build_root / 'zrythm-config.h',
          '--template=gcc',
          '--error-exitcode=1',
          '-DGETTEXT_PACKAGE=zrythm',
          '-DDEPRECATED_MSG(x)=',
          ignore_commands,
          other_files,
          ])
    endif # cppcheck found

    if clang_tidy.found ()
      name = file.underscorify () + '.clang-tidy'
      clang_tidy_targets += custom_target (
        name,
        output: name,
        depends: [ config_h_vcs ],
        command: [
          run_clang_tidy, clang_tidy.full_path (),
          meson_build_root, '@PRIVATE_DIR@',
          file,
          ])
    endif # cppcheck found

    processed_files += file
  endforeach

  run_target (
    'cppcheck',
    command : [
      'echo', 'done',
      ],
    depends: cppcheck_targets)
  run_target (
    'clang-tidy',
    command : [
      'echo', 'done',
      ],
    depends: clang_tidy_targets)
endif
