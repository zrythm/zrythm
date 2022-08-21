# SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

rtkit_cargs = []
if dbus_dep.found ()
  rtkit_cargs += ['-DHAVE_DBUS=1']
endif

rtkit_lib = static_library (
  'rtkit',
  'rtkit.c',
  c_args: rtkit_cargs,
  dependencies: dbus_dep,
  )
