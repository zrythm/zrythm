# Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

manual_po_files = [
  'appendix.po',
  'chords-and-scales.po',
  'configuration.po',
  'contributing.po',
  'credits.po',
  'editing.po',
  'exporting.po',
  'getting-started.po',
  'glossary.po',
  'index.po',
  'invoking-from-command-line.po',
  'mixing.po',
  'modulators.po',
  'playback-and-recording.po',
  'plugins-files.po',
  'projects.po',
  'routing.po',
  'scripting.po',
  'theming.po',
  'tracks.po',
  'user-media.po',
  'zrythm-interface.po',
  ]

# note: if this fails, run the manual_gettext
# target to generate missing .po files
foreach lang, lang_str : locales
  if msgfmt.found () and lang != 'en'
    foreach po_file : manual_po_files
      po_file_path = lang / 'LC_MESSAGES' / po_file
      if fs.exists (po_file_path)
        test (
          'user-manual-' + lang + '-' + po_file,
          msgfmt,
          args: [
            '-v', '-c', files (po_file_path) ,
            ])
        if os_gnu
          foreach illegal_char : illegal_chars
            test (
              'user-manual-' + lang + '-' + po_file + '-illegal-char-' + illegal_char,
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
    endforeach
  endif
endforeach
