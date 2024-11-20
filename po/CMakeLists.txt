# SPDX-FileCopyrightText: © 2019-2020, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

return()

# ================================================================
# Generate POTFILES
# ================================================================

set(potfiles_path "${CMAKE_CURRENT_SOURCE_DIR}/POTFILES.in")
add_custom_target(collect-translatables
  BYPRODUCTS ${potfiles_path}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/collect_translatables.py
    ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${potfiles_path}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/collect_translatables.py
    ${CMAKE_BINARY_DIR}/data/org.zrythm.Zrythm.gschema.xml
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Collecting translatables (generating POTFILES.in)..."
)

# ================================================================
# Generate/update .pot file
# ================================================================

# Custom target to generate/update the .pot file
set(pot_path "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.pot")
add_custom_target(gen-pot
  BYPRODUCTS ${pot_path}
  COMMAND xgettext --keyword=N_ --keyword=_ --keyword=C_:1c,2
    --add-comments --from-code=UTF-8
    --package-name=${prog_name} --package-version=${ZRYTHM_VERSION}
    --msgid-bugs-address=${issue_tracker_url}
    -o ${pot_path} -f ${potfiles_path}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  DEPENDS collect-translatables
  COMMENT "Extracting translatable messages to ${pot_path}"
)

# join appdata translations in pot file
if(ITSTOOL_EXECUTABLE)
  set(appstream_its_rules "${CMAKE_CURRENT_SOURCE_DIR}/its/appdata.its")
  set(appstream_its_rules "${appstream_its_rules}" PARENT_SCOPE)
  set(pot_path2_tmp "${pot_path}.2.in")
  add_custom_command(
    TARGET gen-pot
    POST_BUILD
    # generate a temp .pot file for appdata
    COMMAND ${ITSTOOL_EXECUTABLE} -o ${pot_path2_tmp}
      "--its=${appstream_its_rules}"
      "${CMAKE_BINARY_DIR}/data/org.zrythm.Zrythm.appdata.xml.in"
    # strip comments and location
    COMMAND msgattrib --no-location -o ${pot_path2_tmp} ${pot_path2_tmp}
    # merge into main .pot file
    COMMAND msgcat --use-first ${pot_path} ${pot_path2_tmp} -o ${pot_path}
    # cleanup
    COMMAND ${CMAKE_COMMAND} -E remove "${pot_path2_tmp}"
    COMMENT "Appending XML translations from ${xml_file} into .pot file..."
    DEPENDS "${CMAKE_BINARY_DIR}/data/org.zrythm.Zrythm.appdata.xml.in"
  )
endif()

# ================================================================
# Update .po files from .pot file
# ================================================================

# This will update .po files when explicitly called
# (doesn't run by default)
add_custom_target(update-po)
foreach(lang ${languages_for_gettext})
  add_custom_command(
    TARGET update-po
    BYPRODUCTS ${lang}.po
    COMMAND "${GETTEXT_MSGMERGE_EXECUTABLE}" --quiet
      --update "${CMAKE_CURRENT_SOURCE_DIR}/${lang}.po" ${pot_path}
    DEPENDS gen-pot
    COMMENT "Updating ${lang}.po"
  )
endforeach()

# ================================================================
# Generate .mo files
# ================================================================

# This will compile .po to .gmo files and install them by default
# Creates a target `pofiles`.
foreach(lang ${languages_for_gettext})
  gettext_process_po_files(${lang}
    ALL
    INSTALL_DESTINATION ${CMAKE_INSTALL_LOCALEDIR}
    PO_FILES ${lang}.po
    )
endforeach()

# ================================================================
# Set some global variables
# ================================================================

set(po_files ${languages_for_gettext})
set(mo_files ${languages_for_gettext})
list(TRANSFORM po_files APPEND ".po")
list(TRANSFORM mo_files APPEND ".gmo")
set(source_po_files "${po_files}")
set(build_po_files "${po_files}")
set(build_mo_files "${mo_files}")
list(TRANSFORM source_po_files PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
list(TRANSFORM build_po_files PREPEND "${CMAKE_CURRENT_BINARY_DIR}/")
list(TRANSFORM build_mo_files PREPEND "${CMAKE_CURRENT_BINARY_DIR}/")
set(source_po_files "${source_po_files}" PARENT_SCOPE)
set(build_po_files "${build_po_files}" PARENT_SCOPE)
set(build_mo_files "${build_mo_files}" PARENT_SCOPE)


# ================================================================
# Generate LINGUAS
# ================================================================

# Generate LINGUAS file
set(linguas_file "${CMAKE_CURRENT_SOURCE_DIR}/LINGUAS")
string(REPLACE ";" "\n" languages_newline "${languages_for_gettext}")
file(WRITE ${linguas_file} "${languages_newline}")
