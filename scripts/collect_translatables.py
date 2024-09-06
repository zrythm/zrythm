#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#
# This script prints all the translatable files.

import os
import os.path
import sys

os.chdir(os.getenv('MESON_SOURCE_ROOT', '.'))

potfiles = open (
  'po/POTFILES', 'w')

count = 0
for path in ['inc', 'src', 'resources', 'data']:
  for dirpath, dirnames, filenames in os.walk(path):
    for filename in [
      f for f in filenames if f.endswith('.c') or
      f.endswith('.h') or f.endswith('.ui') or
      f.endswith('.gschema.xml') or
      f.endswith('.desktop.in')]:
        str = os.path.join(dirpath, filename)
        potfiles.write (str + '\n')
        count = count + 1

potfiles.write (os.path.join ('build', 'data', 'org.zrythm.Zrythm.gschema.xml') + '\n')
potfiles.write (os.path.join ('build', 'data', 'org.zrythm.Zrythm.appdata.xml.in'))

print (
  "wrote {} entries to {}".format (
    count, potfiles.name))

potfiles.close ()
