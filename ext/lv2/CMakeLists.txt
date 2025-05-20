# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_library(_zrythm_lv2 INTERFACE)
target_include_directories(_zrythm_lv2 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
add_library(zrythm::lv2 ALIAS _zrythm_lv2)
