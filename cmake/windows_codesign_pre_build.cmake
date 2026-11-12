# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: CC0-1.0

# Signs every PE file in the staged install tree before the Inno Setup
# installer is built, so the files inside the installer carry signatures.
# Signing runs only when CERTUM_EMAIL and CERTUM_OTP are set (see
# scripts/windows_codesign.ps1 for the full environment contract).

if(NOT CPACK_GENERATOR STREQUAL "INNOSETUP")
  return()
endif()

if("$ENV{CERTUM_EMAIL}" STREQUAL "" OR "$ENV{CERTUM_OTP}" STREQUAL "")
  message(STATUS "windows codesign: CERTUM_EMAIL/CERTUM_OTP not set, skipping install tree signing")
  return()
endif()

message(STATUS "windows codesign: signing install tree ${CPACK_TEMPORARY_DIRECTORY}")
execute_process(
  COMMAND powershell -NoProfile -ExecutionPolicy Bypass
    -File "${CMAKE_CURRENT_LIST_DIR}/../scripts/windows_codesign.ps1"
    -Root "${CPACK_TEMPORARY_DIRECTORY}"
  RESULT_VARIABLE result)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "windows codesign: signing the install tree failed (${result})")
endif()
