# SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

set(copy_files
  favicon.ico
  z.svg
)

foreach(f ${copy_files})
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${f}
    ${CMAKE_CURRENT_BINARY_DIR}/${f}
    COPYONLY
  )
endforeach()

add_subdirectory(css)
add_subdirectory(img)
add_subdirectory(js)