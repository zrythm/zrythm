# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_subdirectory(eg-amp.lv2)
add_subdirectory(eg-fifths.lv2)
add_subdirectory(plumbing.lv2)
add_subdirectory(sigabrt.lv2)
add_subdirectory(test-instrument.lv2)

set(test_lv2_plugin_libs ${test_lv2_plugin_libs} PARENT_SCOPE)
set(test_lv2_plugins ${test_lv2_plugins} PARENT_SCOPE)