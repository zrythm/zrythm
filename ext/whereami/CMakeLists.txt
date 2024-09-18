# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_library(whereami STATIC whereami.c)
target_include_directories(whereami PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_library(whereami::whereami ALIAS whereami)