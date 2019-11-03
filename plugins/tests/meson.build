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

lv2lint = find_program ('lv2lint', required: false)
lv2_validate = find_program ('lv2_validate', required: false)
sord_validate = find_program ('sord_validate', required: false)

plugin_tests = [
  ['ZLFO', zlfo_ttl, zlfo_build_dir,
    zlfo_cdata.get ('LFO_URI')],
  ]

foreach plugin_test : plugin_tests
  suite = ['plugin:' + plugin_test[0]]

  if (lv2lint.found())
    test (
      'LV2 lint', lv2lint,
      args: [
        '-I', plugin_test[2] + '/',
        plugin_test[3]],
      suite: suite)
  endif

  # FIXME this fails
  #if (lv2_validate.found())
    #test (
      #'LV2 validate', lv2_validate,
      #args: [plugin_test[1]],
      #suite: suite)
  #endif
endforeach
