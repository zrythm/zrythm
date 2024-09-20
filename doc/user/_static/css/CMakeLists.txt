# SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/custom.css
    COMMAND ${SASS_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/custom_css.scss ${CMAKE_CURRENT_BINARY_DIR}/custom.css
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/custom_css.scss
    COMMENT "Building custom.css"
)
add_custom_target(custom_css_target DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/custom.css)