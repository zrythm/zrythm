# SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

set(app_id "org.zrythm.Zrythm")

# Install samples
set(samples
  ${CMAKE_CURRENT_SOURCE_DIR}/samples/klick/square_emphasis.wav
  ${CMAKE_CURRENT_SOURCE_DIR}/samples/klick/square_normal.wav
)
install(FILES ${samples}
  DESTINATION share/zrythm/samples COMPONENT Runtime)

# Configure and install launcher scripts
if(UNIX)
  if(ZRYTHM_IS_FLATPAK)
    set(FLATPAK_BUILD_ONE_OR_ZERO 1)
  else()
    set(FLATPAK_BUILD_ONE_OR_ZERO 0)
  endif()
  if(ZRYTHM_IS_INSTALLER_VER)
    set(ZRYTHM_IS_INSTALLER_VER_ONE_OR_ZERO 1)
  else()
    set(ZRYTHM_IS_INSTALLER_VER_ONE_OR_ZERO 0)
  endif()
  if(APPLE)
    set(LD_LIBRARY_PATH_ENV "DYLD_LIBRARY_PATH")
  else()
    set(LD_LIBRARY_PATH_ENV "LD_LIBRARY_PATH")
  endif()
  configure_file(zrythm_launch.in ${prog_name_lowercase}_launch @ONLY)
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${prog_name_lowercase}_launch
    TYPE BIN
    COMPONENT Runtime
  )
endif()

if(OS_GNU)
  configure_file(zrythm_gdb.in ${prog_name_lowercase}_gdb @ONLY)
  configure_file(zrythm_valgrind.in ${prog_name_lowercase}_valgrind @ONLY)
  install(PROGRAMS
    ${CMAKE_CURRENT_BINARY_DIR}/${prog_name_lowercase}_gdb
    ${CMAKE_CURRENT_BINARY_DIR}/${prog_name_lowercase}_valgrind
    DESTINATION ${CMAKE_INSTALL_BINDIR})

  # Desktop entry (FIXME/TODO)
  set(desktop_entry_name ${prog_name})
  if(ZRYTHM_IS_INSTALLER_VER)
    set(desktop_entry_name "${desktop_entry_name} beta")
  endif()
  configure_file(zrythm.desktop.in zrythm.desktop.in @ONLY)
  set(desktop_file_in "${CMAKE_CURRENT_BINARY_DIR}/zrythm.desktop.in")
  set(desktop_file_out "${app_id}.desktop")
  add_custom_command(
    OUTPUT ${desktop_file_out}
      COMMAND
        "${GETTEXT_MSGFMT_EXECUTABLE}" --desktop
        --template="${desktop_file_in}"
        -d "${CMAKE_SOURCE_DIR}/po"
        -o "${desktop_file_out}"
      DEPENDS "${desktop_file_in}"
      COMMENT "Generating desktop file"
    )
  add_custom_target(desktop-file DEPENDS ${desktop_file_out})
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${desktop_file_out}"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    COMPONENT Runtime
  )
endif()

# Appdata
set(release_tag_base_url "https://gitlab.zrythm.org/zrythm/zrythm/-/tags/")
set(homepage_url "https://www.zrythm.org/en/index.html")
set(translate_url "https://hosted.weblate.org/engage/zrythm/")
set(contact_url "https://www.zrythm.org/en/community.html#contact")
set(main_screenshot_url "https://www.zrythm.org/static/images/screenshots/screenshot-20240208.png")
configure_file(gen-appdata-xml.py gen-appdata-xml.py @ONLY)
set(appdata_xml_filename "${app_id}.appdata.xml")
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${appdata_xml_filename}.in"
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/gen-appdata-xml.py
    "${CMAKE_CURRENT_BINARY_DIR}/${appdata_xml_filename}.in.in" ${app_id}
  COMMAND "${XMLLINT_EXECUTABLE}" --format --output
    "${CMAKE_CURRENT_BINARY_DIR}/${appdata_xml_filename}.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${appdata_xml_filename}.in.in"
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/gen-appdata-xml.py
  COMMENT "Generating appdata XML"
)
add_custom_target(appdata-xml ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${appdata_xml_filename}.in")

if(ITSTOOL_EXECUTABLE AND FALSE)
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${appdata_xml_filename}"
    COMMAND
    ${ITSTOOL_EXECUTABLE}
      --join "${CMAKE_CURRENT_BINARY_DIR}/${appdata_xml_filename}.in"
      "--its=${appstream_its_rules}"
      ${build_mo_files}
      --out "${CMAKE_CURRENT_BINARY_DIR}/${appdata_xml_filename}"
    DEPENDS appdata-xml pofiles
    COMMENT "Merging translations for appdata XML"
    )
else()
  message("itstool not found, not merging translations for appdata XML")
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${appdata_xml_filename}"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/${appdata_xml_filename}.in"
      "${CMAKE_CURRENT_BINARY_DIR}/${appdata_xml_filename}"
    DEPENDS appdata-xml
    COMMENT "Copying untranslated appdata XML"
  )
endif()
add_custom_target(appdata-xml-merged DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${appdata_xml_filename}")
set(APPDATA_FILE "${CMAKE_CURRENT_BINARY_DIR}/${appdata_xml_filename}" PARENT_SCOPE)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${appdata_xml_filename}
  DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
  COMPONENT Runtime
)

# Validate the appdata file
find_program(APPSTREAM_UTIL_EXECUTABLE appstream-util)
if(APPSTREAM_UTIL_EXECUTABLE)
  add_test(
    NAME appstream-util-validate-appdata
    COMMAND ${APPSTREAM_UTIL_EXECUTABLE} validate-relax --nonet ${CMAKE_CURRENT_BINARY_DIR}/${appdata_xml_filename}
    )
    set_tests_properties(appstream-util-validate-appdata
    PROPERTIES
    LABELS "data"
    DEPENDS appdata-xml-merged
    )
endif()

find_program(APPSTREAMCLI_EXECUTABLE appstreamcli)
if(APPSTREAMCLI_EXECUTABLE)
  add_test(
    NAME appstream-cli-validate-appdata
    COMMAND ${APPSTREAMCLI_EXECUTABLE} validate ${CMAKE_CURRENT_BINARY_DIR}/${appdata_xml_filename}
    )
    set_tests_properties(appstream-cli-validate-appdata
    PROPERTIES
    LABELS "data"
    DEPENDS appdata-xml-merged
    )
endif()

if(APPLE)
  configure_file(zrythm_lldb.in ${prog_name_lowercase}_lldb @ONLY)
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${prog_name_lowercase}_lldb
    DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

add_subdirectory(icon-themes)

# Zrythm project MIME type
install(FILES org.zrythm.Zrythm-mime.xml
  DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages
  COMPONENT Runtime
)

# Templates
install(DIRECTORY DESTINATION ${zrythm_datadir}/templates)

# CSS themes
# add_subdirectory(css-themes)

add_subdirectory(plugins)
set(BUNDLED_PLUGINS_FOR_TESTS "${BUNDLED_PLUGINS_FOR_TESTS}" PARENT_SCOPE)
set(BUNDLED_PLUGINS_FOR_TESTS_TGTS "${BUNDLED_PLUGINS_FOR_TESTS_TGTS}" PARENT_SCOPE)

add_subdirectory(windows)
