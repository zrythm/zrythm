# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_executable(zrythm-engine
  main.cpp
  audio_engine_application.h
  audio_engine_application.cpp
)

target_link_libraries(zrythm-engine PRIVATE zrythm_dsp_lib)
target_link_options(zrythm-engine PRIVATE ${zrythm_link_args})
set_target_properties(zrythm-engine PROPERTIES
  WIN32_EXECUTABLE TRUE
)

install(TARGETS zrythm-engine RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
