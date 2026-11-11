# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: CC0-1.0

# CPack loads this file before it initializes the generator (see
# CPACK_PROJECT_CONFIG_FILE). It defines a Sign Tool for the Inno Setup
# compiler, which then signs the installer, the uninstaller and Setup's
# temporary self-copies by invoking scripts/windows_codesign.ps1 (see that
# file for the environment contract). Signing runs only when
# WINDOWS_CODESIGN_CERT_THUMBPRINT is set, matching the install-tree
# signing in cmake/windows_codesign_pre_build.cmake.

if(NOT CPACK_GENERATOR STREQUAL "INNOSETUP")
  return()
endif()

if("$ENV{WINDOWS_CODESIGN_CERT_THUMBPRINT}" STREQUAL "")
  message(STATUS
    "windows codesign: WINDOWS_CODESIGN_CERT_THUMBPRINT not set, skipping installer and uninstaller signing")
  return()
endif()

# resolves the "../" in the relative path below
get_filename_component(
  _zrythm_codesign_script
  "${CMAKE_CURRENT_LIST_DIR}/../scripts/windows_codesign.ps1"
  REALPATH)

set(CPACK_INNOSETUP_SETUP_SignTool "zrythm_codesign")
# "yes" is the default when SignTool is set; stated explicitly because the
# uninstaller signature is the purpose of this configuration
set(CPACK_INNOSETUP_SETUP_SignedUninstaller "yes")

# ISCC expands $q to a quote and $f to the quoted file to sign; the quotes
# around the whole argument keep it a single argv element
set(CPACK_INNOSETUP_EXECUTABLE_ARGUMENTS
  "\"/Szrythm_codesign=powershell -NoProfile -ExecutionPolicy Bypass -File $q${_zrythm_codesign_script}$q -Files $f\"")
