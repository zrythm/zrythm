# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_executable(zrythm-engine
  WIN32 # not a GUI application but build fails otherwise
  main.cpp
  audio_engine_application.h
  audio_engine_application.cpp
)

target_link_libraries(zrythm-engine PRIVATE zrythm_dsp_lib)