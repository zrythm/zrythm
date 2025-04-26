# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_library(_rubberband_lib STATIC)
target_sources(_rubberband_lib PRIVATE single/RubberBandSingle.cpp)
target_include_directories(_rubberband_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
add_library(zrythm::rubberband ALIAS _rubberband_lib)
