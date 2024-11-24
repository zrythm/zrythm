# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

if(ZRYTHM_TESTS)
  # use custom lv2apply because upstream doesn't have URID support
  add_executable(zrythm_lv2apply
    lv2apply.cpp
  )
  target_link_libraries(zrythm_lv2apply PRIVATE
    SndFile::sndfile
    PkgConfig::LILV
    zrythm_utils_lib
  )
endif()
