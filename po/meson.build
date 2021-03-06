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

i18n = import('i18n')

# define GETTEXT_PACKAGE
add_project_arguments (
  '-DGETTEXT_PACKAGE="' + meson.project_name () + '"',
  language:'c',
  )
i18n.gettext (
  meson.project_name(),
  args: [
    '--directory=' + meson_src_root,
    '--msgid-bugs-address=https://redmine.zrythm.org/projects/zrythm/issues/new',
    '--add-comments',
    '--keyword=__',
    '--keyword=C_:1c,2',
    ],
  )

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
  if msgfmt.found () and lang != 'en'
    test (
      lang + '-po', msgfmt,
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
