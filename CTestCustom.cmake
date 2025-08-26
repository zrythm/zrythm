# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

list(APPEND CTEST_CUSTOM_TESTS_IGNORE
  # these are from soxr
  20-bit-perfect-44100-192000
  20-bit-perfect-192000-44100
  20-bit-perfect-44100-65537
  20-bit-perfect-65537-44100
  28-bit-perfect-44100-192000
  28-bit-perfect-192000-44100
  28-bit-perfect-44100-65537
  28-bit-perfect-65537-44100
)

# TODO: add tests to ignore from Memcheck here
set(CTEST_CUSTOM_MEMCHECK_IGNORE
  ${CTEST_CUSTOM_MEMCHECK_IGNORE}
  # Example-vtkLocal
)

# ignore everything under build_*/.qt and build_*/_deps
set(CTEST_CUSTOM_COVERAGE_EXCLUDE
    ".*/\\.qt/.*"
    ".*/_deps/.*"
    ".*/build_.*/.*_autogen/.*"
    ".*/qrc_.*\\.cpp"
    ".*/\\.rcc/.*"
    ".*/ext/.*"
    # these 2 give errors
    ".*/src/utils/format\\.h"
    ".*/src/utils/logger\\.h"
    ".*/tests/.*"
)
