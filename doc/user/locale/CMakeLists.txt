# SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# Copy locales to builddir
set(copy_locales_targets)

# Note: if this fails (such as when adding a new language),
# run the manual_gettext target to generate missing .po files
foreach(lang ${locales_list})
  if(NOT ${lang} STREQUAL "en")
    set(po_file "zrythm-manual.po")
    set(po_file_parent "${lang}/LC_MESSAGES")
    set(po_file_path "${lang}/LC_MESSAGES/${po_file}")

    # Copy to build dir
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${po_file_path}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/${po_file_parent}
      COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/${po_file_path}
        ${CMAKE_CURRENT_BINARY_DIR}/${po_file_path}
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${po_file_path}
    )

    add_custom_target(copy_${lang}_messages
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${po_file_path}
    )
    list(APPEND copy_locales_targets copy_${lang}_messages)

    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${po_file_path})
      add_test(
        NAME user-manual-${lang}-po
        COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -v -c ${CMAKE_CURRENT_SOURCE_DIR}/${po_file_path}
      )
      set_tests_properties(user-manual-${lang}-po PROPERTIES LABELS "po")

      if(OS_GNU)
        foreach(illegal_char ${illegal_chars})
          add_test(
            NAME user-manual-${lang}-po-illegal-char-${illegal_char}
            COMMAND ${CMAKE_COMMAND} -E env LANG=C
              grep -n "[${illegal_char}]" ${CMAKE_CURRENT_SOURCE_DIR}/${po_file_path}
          )
          set_tests_properties(user-manual-${lang}-po-illegal-char-${illegal_char} PROPERTIES 
            LABELS "po"
            PASS_REGULAR_EXPRESSION "^$"
            FAIL_REGULAR_EXPRESSION "."
          )
        endforeach()
      endif()
    else()
      message(WARNING "File ${po_file_path} does not exist")
    endif()
  endif()
endforeach()

add_custom_target(copy_locales_targets DEPENDS ${copy_locales_targets})
