# SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

find_package(spdlog QUIET)
if(spdlog_FOUND)
  target_compile_definitions(spdlog::spdlog INTERFACE
    SPDLOG_FMT_EXTERNAL
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
  )
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_definitions(spdlog::spdlog INTERFACE
      SPDLOG_FUNCTION=__PRETTY_FUNCTION__
    )
  else()
    target_compile_definitions(spdlog::spdlog INTERFACE
      SPDLOG_FUNCTION=__FUNCTION__
    )
  endif()
else()
  FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    # 1.15.0
    GIT_TAG 8e5613379f5140fefb0b60412fbf1f5406e7c7f8
  )
  set(SPDLOG_FMT_EXTERNAL ON)
  FetchContent_MakeAvailable(spdlog)
  target_compile_definitions(spdlog INTERFACE
    SPDLOG_FMT_EXTERNAL
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
  )
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_definitions(spdlog INTERFACE
      SPDLOG_FUNCTION=__PRETTY_FUNCTION__
    )
  else()
    target_compile_definitions(spdlog INTERFACE
      SPDLOG_FUNCTION=__FUNCTION__
    )
  endif()
endif()
