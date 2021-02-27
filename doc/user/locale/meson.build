# Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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
# Test PO files for errors.

# copy locales to builddir
copy_locales_targets = []

# note: if this fails, run the manual_gettext
# target to generate missing .po files
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
        args: [
          '-v', '-c', files (po_file_path) ,
          ])
      if os_gnu
        foreach illegal_char : illegal_chars
          test (
            'user-manual-' + lang + '-po-illegal-char-' + illegal_char,
            bash,
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
