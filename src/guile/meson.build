# SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

guile_snarfable_srcs = [
  meson.current_source_dir() / 'project.c',
  meson.current_source_dir() / 'zrythm.c',
  ]

subdir ('actions')
subdir ('audio')
subdir ('plugins')

guile_srcs = [
  'guile.c',
  'project_generator.c',
  guile_snarfable_srcs,
  ]

if have_guile
  if guile_snarf.found ()
    # .x files
    guile_snarfed_srcs = []
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
      new_file = without_ext.split (
        meson.current_source_dir () + '/')[1]
      new_file = new_file.underscorify() + '.x'
      guile_snarfed_srcs += custom_target (
        new_file,
        output: new_file,
        input: snarfable_src,
        command: [
          guile, '-s',
          meson_build_root / 'scripts/guile-snarf-wrap.scm',
          guile_dep.name(),
          '@OUTPUT@', '@INPUT@', ],
        )
    endforeach

    guile_srcs += guile_snarfed_srcs
  endif

  guilelib = static_library (
    'guilelib',
    guile_srcs,
    dependencies: [ guile_dep, zrythm_deps ],
    c_args: [ common_cflags ],
    include_directories: all_inc,
    )
endif
