# SPDX-FileCopyrightText: © 2019-2020 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

languages_for_gettext = []
foreach l_key, l_mapping : locales
  if l_key != 'en'
    languages_for_gettext += l_key
  endif
endforeach
i18n.gettext (
  meson.project_name(),
  languages: languages_for_gettext,
  args: [
    '--package-name=Zrythm',
    '--package-version=' + meson.project_version (),
    '--directory=' + meson_src_root,
    '--msgid-bugs-address=' + cdata.get ('ISSUE_TRACKER_URL'),
    '--add-comments',
    '--keyword=N_',
    '--keyword=C_:1c,2',
    '--from-code=UTF-8',
    ],
  )

# create same directory structure in build/po for use
# with i18n.merge_file()
foreach gettext_l : languages_for_gettext
  configure_file (
    copy: true, input: gettext_l + '.po',
    output: '@PLAINNAME@')
endforeach
configure_file (
  output: 'LINGUAS',
  command: [
    bash, '-c',
    'echo "' + ' '.join (languages_for_gettext) + '" > @OUTPUT@'])

collect_translatables = run_target (
  'collect-translatables',
  command: [
    meson_src_root / 'scripts/collect_translatables.sh',
    ],
  )

# create locales.h
ll_langs = ''
lang_codes = ''
lang_strings = ''
lang_strings_w_codes = ''
foreach lang, lang_str : locales
  ll_langs += '  LL_' + lang.to_upper () + ',\n'
  lang_codes += '  "' + lang + '",\n'
  lang_strings += '  "' + lang_str + '",\n'
  lang_strings_w_codes += '  "' + lang_str + ' [' + lang + ']",\n'
endforeach
locales_h = configure_file (
  input: 'locales.h.in',
  output: 'zrythm-locales.h',
  configuration: {
    'LANG_CODES': lang_codes,
    'LANG_STRINGS': lang_strings,
    'LANG_STRINGS_W_CODES': lang_strings_w_codes,
    })

# Test PO files for errors.
foreach lang, lang_str : locales
  if msgfmt.found () and lang != 'en' and fs.is_file (lang + '.po')
    test (
      lang + '-po', msgfmt,
      suite: 'i18n',
      args: [ '-v', '-c', files (lang + '.po') ])
    if os_gnu
      foreach illegal_char : illegal_chars
        test (
          lang + '-po-illegal-char-' + illegal_char,
          bash,
          args: [
            illegal_char_test_args, illegal_char,
            files (lang + '.po') ,
            ])
      endforeach
    endif
  endif
endforeach
