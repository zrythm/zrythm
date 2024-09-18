# SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# Install precompiled .css if available or compile
# and install .scss
set(css_theme_names
  "zrythm" #"fruity"
)

foreach(css_theme_name ${css_theme_names})
  set(css_theme_file "${css_theme_name}-theme.css")
  if(EXISTS ${css_theme_file})
    install(FILES ${css_theme_file}
      DESTINATION ${themes_css_dir})
  else()
    find_program(SASSC_EXECUTABLE sassc REQUIRED)
    add_custom_command(
      OUTPUT ${css_theme_file}
      COMMAND ${SASSC_EXECUTABLE} --sass --omit-map-comment
        --style nested
        ${CMAKE_CURRENT_SOURCE_DIR}/${css_theme_name}/theme.scss
        ${css_theme_file}
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${css_theme_name}/theme.scss
      COMMENT "Generating ${css_theme_file}"
    )
    add_custom_target(${css_theme_name}-theme-css ALL
      DEPENDS ${css_theme_file})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${css_theme_file}
      DESTINATION ${themes_css_dir})
  endif()
endforeach()
