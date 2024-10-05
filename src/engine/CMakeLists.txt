# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_executable(zrythm-engine
  main.cpp
  audio_engine.h
)

target_link_libraries(zrythm-engine PRIVATE zrythm_lib)
target_link_options(zrythm-engine PRIVATE ${zrythm_link_args})

install(TARGETS zrythm-engine RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
