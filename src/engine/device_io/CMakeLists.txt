# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_gui_lib
  PRIVATE
    engine.h
    engine.cpp
    engine_dummy.h
    engine_dummy.cpp
    engine_jack.h
    engine_jack.cpp
    ext_port.h
    ext_port.cpp
    hardware_processor.h
    hardware_processor.cpp
)