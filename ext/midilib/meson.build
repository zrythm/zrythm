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

endianess_flag=''
if host_machine.endian () == 'big'
  endianess_flag = '-DHAVE_BIG_ENDIAN=1'
elif host_machine.endian () == 'little'
  endianess_flag = '-DHAVE_LITTLE_ENDIAN=1'
endif

midilib = static_library (
  'midilib',
  join_paths ('src', 'midifile.c'),
  join_paths ('src', 'midiutil.c'),
  c_args: [
    endianess_flag,
    ],
  )
