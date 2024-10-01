# SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

pkg_check_modules(VAMP_SDK REQUIRED IMPORTED_TARGET vamp-sdk)

if(WIN32 AND MINGW)
  # force static in msys2 otherwise linking fails (badly packaged most likely)
  # TODO drop dependency on vamp, use essentia or aubio instead which are more popular and have sane build systems
  add_library(vamp-sdk::vamp-sdk STATIC)
  target_include_directories(vamp-sdk::vamp-sdk PUBLIC ${VAMP_SDK_STATIC_INCLUDE_DIRS})
  target_link_libraries(vamp-sdk::vamp-sdk PUBLIC ${VAMP_SDK_STATIC_LIBRARIES})
  target_compile_options(vamp-sdk::vamp-sdk PUBLIC ${VAMP_SDK_STATIC_CFLAGS_OTHER})
  target_link_options(vamp-sdk::vamp-sdk PUBLIC ${VAMP_SDK_STATIC_LDFLAGS})
else()
  add_library(vamp-sdk::vamp-sdk ALIAS PkgConfig::VAMP_SDK)
endif()

add_library(vamp_plugins_lib STATIC
  FixedTempoEstimator.cpp
  BeatTrack.cpp
)
target_link_libraries(vamp_plugins_lib
  PRIVATE
    qm_dsp_lib
    vamp-sdk::vamp-sdk
)
target_include_directories(vamp_plugins_lib
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
# set_target_properties(vamp_plugins PROPERTIES
#   POSITION_INDEPENDENT_CODE ON
# )
