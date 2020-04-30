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
    '--msgid-bugs-address=https://git.zrythm.org/zrythm/zrythm/issues',
    '--add-comments',
    '--keyword=__',
    ],
  )

collect_translatables = run_target (
  'collect-translatables',
  command: [
    meson.source_root () / 'scripts' / 'collect_translatables.sh',
    ],
  )
