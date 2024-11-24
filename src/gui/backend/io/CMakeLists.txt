# SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

target_sources(zrythm_lib
  PRIVATE
    file_descriptor.cpp
    file_import.cpp
    midi_file.cpp)

add_subdirectory(serialization)
