# SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

set(app_id "org.zrythm.Zrythm")

# Prepare icons
if(APPLE)
  set(_iconset_dir "${CMAKE_CURRENT_BINARY_DIR}/Zrythm.iconset")
  file(MAKE_DIRECTORY "${_iconset_dir}")

  # --- Generate all PNG sizes (with padding) directly from SVG ---
  set(_icon_sizes 16 32 64 128 256 512)
  foreach(SIZE IN LISTS _icon_sizes)
    math(EXPR SIZE_2X "${SIZE} * 2")
    math(EXPR PADDING "(100 * ${SIZE}) / 1024")
    math(EXPR PADDING_2X "(100 * ${SIZE_2X}) / 1024")
    math(EXPR INNER_ICON_SIZE "${SIZE} - 2 * ${PADDING}")
    math(EXPR INNER_ICON_SIZE_2X "${SIZE_2X} - 2 * ${PADDING_2X}")
    add_custom_command(
      OUTPUT
        "${_iconset_dir}/icon_${SIZE}x${SIZE}.png"
        "${_iconset_dir}/icon_${SIZE}x${SIZE}@2x.png"
      COMMAND ${RSVG_CONVERT_EXECUTABLE}
        -w ${INNER_ICON_SIZE} -h ${INNER_ICON_SIZE}
        --page-width ${SIZE} --page-height ${SIZE}
        --top ${PADDING} --left ${PADDING}
        "${ZRYTHM_SVG_ICON}" -o "${_iconset_dir}/icon_${SIZE}x${SIZE}.png"
      COMMAND ${RSVG_CONVERT_EXECUTABLE}
        -w ${INNER_ICON_SIZE_2X} -h ${INNER_ICON_SIZE_2X}
        --page-width ${SIZE_2X} --page-height ${SIZE_2X}
        --top ${PADDING_2X} --left ${PADDING_2X}
        "${ZRYTHM_SVG_ICON}" -o "${_iconset_dir}/icon_${SIZE}x${SIZE}@2x.png"
      COMMENT "Generating ${SIZE}x${SIZE} icons..."
    )
  endforeach()

  # --- Generate .icns ---
  add_custom_command(
    OUTPUT "${ZRYTHM_ICNS_FILE_PATH}"
    COMMAND iconutil -c icns "${_iconset_dir}" -o "${ZRYTHM_ICNS_FILE_PATH}"
    DEPENDS
      "${_iconset_dir}/icon_16x16.png"
      "${_iconset_dir}/icon_16x16@2x.png"
      "${_iconset_dir}/icon_32x32.png"
      "${_iconset_dir}/icon_32x32@2x.png"
      "${_iconset_dir}/icon_64x64.png"
      "${_iconset_dir}/icon_64x64@2x.png"
      "${_iconset_dir}/icon_128x128.png"
      "${_iconset_dir}/icon_128x128@2x.png"
      "${_iconset_dir}/icon_256x256.png"
      "${_iconset_dir}/icon_256x256@2x.png"
      "${_iconset_dir}/icon_512x512.png"
      "${_iconset_dir}/icon_512x512@2x.png"
    COMMENT "Creating .icns file..."
  )

  install(
    FILES "${ZRYTHM_ICNS_FILE_PATH}"
    DESTINATION "$<TARGET_BUNDLE_DIR_NAME:zrythm>/Contents/Resources"
    COMPONENT Runtime
  )

  add_custom_target(generate_icns ALL DEPENDS "${ZRYTHM_ICNS_FILE_PATH}")
elseif(WIN32)
  # --- Convert SVG → BPM for installer banner ---
  add_custom_command(
    OUTPUT "${ZRYTHM_BMP_FILE}"
    COMMAND "${MAGICK_EXECUTABLE}" convert "${ZRYTHM_SVG_ICON}"
      -resize 240x459
      -background white  # Fallback for transparent SVGs
      -flatten
      -define bmp:format=bmp3  # Windows 3.x format
      "${ZRYTHM_BMP_FILE}"
    COMMENT "Generating wizard bitmap..."
    VERBATIM
  )
  add_custom_target(generate_ico ALL DEPENDS "${ZRYTHM_BMP_FILE}")
endif()

# Install samples
install(
  FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/samples/klick/square_emphasis.wav
    ${CMAKE_CURRENT_SOURCE_DIR}/samples/klick/square_normal.wav
  DESTINATION $<IF:$<PLATFORM_ID:Darwin>,$<TARGET_BUNDLE_DIR_NAME:zrythm>/Contents/,>${CMAKE_INSTALL_DATADIR}/zrythm/samples
  COMPONENT Runtime
)

# Configure and install launcher scripts
if(OS_GNU)
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
if(OS_GNU)
  set(release_tag_base_url "https://gitlab.zrythm.org/zrythm/zrythm/-/tags/")
  set(homepage_url "https://www.zrythm.org/en/index.html")
  set(translate_url "https://hosted.weblate.org/engage/zrythm/")
  set(contact_url "https://www.zrythm.org/en/community.html#contact")
  set(main_screenshot_url "https://www.zrythm.org/static/images/screenshots/screenshot-20240208.png")
  configure_file(gen-appdata-xml.py gen-appdata-xml.py @ONLY)
  set(appdata_xml_filename "${app_id}.appdata.xml")
  set(_final_appdata_xml_filepath "${CMAKE_CURRENT_BINARY_DIR}/${appdata_xml_filename}")
  find_program(XMLLINT_EXECUTABLE "xmllint" REQUIRED)
  add_custom_command(
    OUTPUT "${_final_appdata_xml_filepath}.in"
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/gen-appdata-xml.py
      "${_final_appdata_xml_filepath}.in.in" ${app_id}
    COMMAND "${XMLLINT_EXECUTABLE}" --format --output
      "${_final_appdata_xml_filepath}.in"
      "${_final_appdata_xml_filepath}.in.in"
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/gen-appdata-xml.py
    COMMENT "Generating appdata XML"
  )
  if(ITSTOOL_EXECUTABLE AND FALSE) # TODO
    add_custom_command(
      OUTPUT "${_final_appdata_xml_filepath}"
      COMMAND
      ${ITSTOOL_EXECUTABLE}
        --join "${_final_appdata_xml_filepath}.in"
        "--its=${appstream_its_rules}"
        ${build_mo_files}
        --out "${_final_appdata_xml_filepath}"
      DEPENDS "${_final_appdata_xml_filepath}.in" pofiles
      COMMENT "Merging translations for appdata XML"
      )
  else()
    message("itstool not found, not merging translations for appdata XML")
    add_custom_command(
      OUTPUT "${_final_appdata_xml_filepath}"
      COMMAND ${CMAKE_COMMAND} -E copy "${_final_appdata_xml_filepath}.in"
        "${_final_appdata_xml_filepath}"
      DEPENDS "${_final_appdata_xml_filepath}.in"
      COMMENT "Copying untranslated appdata XML"
    )
  endif()
  add_custom_target(appdata-xml DEPENDS "${_final_appdata_xml_filepath}")

  install(FILES "${_final_appdata_xml_filepath}"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
    COMPONENT Runtime
  )

  # Validate the appdata file
  find_program(APPSTREAM_UTIL_EXECUTABLE appstream-util)
  if(APPSTREAM_UTIL_EXECUTABLE)
    add_test(
      NAME appstream-util-validate-appdata
      COMMAND ${APPSTREAM_UTIL_EXECUTABLE} validate-relax --nonet ${_final_appdata_xml_filepath}
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
      COMMAND ${APPSTREAMCLI_EXECUTABLE} validate ${_final_appdata_xml_filepath}
      )
      set_tests_properties(appstream-cli-validate-appdata
      PROPERTIES
      LABELS "data"
      DEPENDS appdata-xml-merged
      )
  endif()
endif() # OS_GNU (appdata XML generation/install/tests)

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

add_subdirectory(platform)
