# SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

pkg_check_modules(VAMP_SDK REQUIRED IMPORTED_TARGET vamp-sdk)

add_library(vamp_plugins_lib STATIC
  FixedTempoEstimator.cpp
  BeatTrack.cpp
)
target_link_libraries(vamp_plugins_lib
  PRIVATE
    qm_dsp_lib
    PkgConfig::VAMP_SDK
)
target_include_directories(vamp_plugins_lib
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
# set_target_properties(vamp_plugins PROPERTIES
#   POSITION_INDEPENDENT_CODE ON
# )
