# Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
    '--directory=' + meson.source_root(),
    '--msgid-bugs-address=https://redmine.zrythm.org/projects/zrythm/issues/new',
    '--add-comments',
    '--keyword=__',
    '--keyword=C_:1c,2',
    ],
  )

collect_translatables = run_target (
  'collect-translatables',
  command: [
    meson.source_root () / 'scripts' / 'collect_translatables.sh',
    ],
  )

# Test PO files for errors.
foreach lang : locales
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
