# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# Renders the window icon SVG to PNG files at build time (see main.cpp)
add_executable(render-icon main.cpp)
target_link_libraries(render-icon PRIVATE zrythm::qt_libs)
set_target_properties(render-icon PROPERTIES DISABLE_PRECOMPILE_HEADERS ON)
