# SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# copy locales to builddir
copy_locales_targets = []

# note: if this fails (such as when adding a new language),
# run the manual_gettext target to generate missing .po files
foreach lang, lang_str : locales
  if msgfmt.found () and lang != 'en'
    po_file = 'zrythm-manual.po'
    po_file_parent = lang / 'LC_MESSAGES'
    po_file_path = lang / 'LC_MESSAGES' / po_file

    # copy to build dir
    copy_locales_targets += custom_target (
      'copy ' + lang + ' messages',
      output: 'copy_' + lang + '_messages',
      command: [
        'mkdir', '-p',
        meson.current_build_dir () / po_file_parent,
        '&&',
        'cp',
        meson.current_source_dir () / po_file_path,
        meson.current_build_dir () / po_file_path,
        '&&', 'touch', '@OUTPUT@',
        ],
      )

    if fs.exists (po_file_path)
      test (
        'user-manual-' + lang + '-po',
        msgfmt,
        suite: 'po',
        args: [
          '-v', '-c', files (po_file_path) ,
          ])
      if os_gnu
        foreach illegal_char : illegal_chars
          test (
            'user-manual-' + lang + '-po-illegal-char-' + illegal_char,
            bash,
            suite: 'po',
            args: [
              illegal_char_test_args, illegal_char,
              files (po_file_path) ,
              ])
        endforeach
      endif
    else
      warning ('file ' + po_file_path + ' does not exist')
    endif
  endif
endforeach
