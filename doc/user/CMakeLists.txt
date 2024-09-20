# SPDX-FileCopyrightText: © 2019-2020, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

if(NOT ZRYTHM_USER_MANUAL)
  return()
endif()

find_program(SPHINX_BUILD_EXECUTABLE NAMES "sphinx-build" "sphinx-build-3" REQUIRED)
find_program(SPHINX_INTL_EXECUTABLE "sphinx-intl" REQUIRED)
find_program(SASS_EXECUTABLE "sass" REQUIRED)

if(NOT SPHINX_BUILD_EXECUTABLE OR NOT SPHINX_INTL_EXECUTABLE OR NOT SASS_EXECUTABLE)
  message(FATAL_ERROR "sphinx-build/sphinx-intl/sass not found - user manual cannot be built")
endif()

# Construct the paths based on the Gettext installation directory
get_filename_component(GETTEXT_BIN_DIR ${GETTEXT_MSGMERGE_EXECUTABLE} DIRECTORY)

set(MSGATTRIB_EXECUTABLE ${GETTEXT_BIN_DIR}/msgattrib)
set(MSGUNIQ_EXECUTABLE ${GETTEXT_BIN_DIR}/msguniq)
set(MSGGREP_EXECUTABLE ${GETTEXT_BIN_DIR}/msggrep)

find_program(MSGATTRIB_EXECUTABLE msgattrib REQUIRED)
find_program(MSGUNIQ_EXECUTABLE msguniq REQUIRED)
find_program(MSGGREP_EXECUTABLE msggrep REQUIRED)

set(SPHINX_BUILDDIR ${CMAKE_CURRENT_BINARY_DIR}/_build)
set(SPHINX_BUILD_OPTS)
if(ZRYTHM_STRICT_SPHINX_OPTS)
  set(SPHINX_BUILD_OPTS -E -W -T)
endif()

set(HTML_TEMPLATES
  ${CMAKE_CURRENT_SOURCE_DIR}/_templates/page.html
)

add_subdirectory(locale)

set(src_rst_files
  index.rst
  appendix/environment.rst
  appendix/files-and-directories.rst
  appendix/gnu-free-documentation-license.rst
  appendix/intro.rst
  chords-and-scales/intro.rst
  chords-and-scales/chord-pad.rst
  configuration/device-setup.rst
  configuration/intro.rst
  configuration/preferences.rst
  configuration/shortcuts.rst
  contributing/intro.rst
  contributing/overview.rst
  contributing/reporting-bugs.rst
  credits/intro.rst
  editing/audio-editor.rst
  editing/automation-editor.rst
  editing/chord-editor.rst
  editing/common-operations.rst
  editing/edit-tools.rst
  editing/editor-toolbars.rst
  editing/intro.rst
  editing/piano-roll.rst
  editing/timeline.rst
  exporting/audio-and-midi.rst
  exporting/intro.rst
  exporting/routing-graph.rst
  getting-started/faq.rst
  getting-started/getting-plugins.rst
  getting-started/intro.rst
  getting-started/installation.rst
  getting-started/running-zrythm.rst
  getting-started/system-requirements.rst
  glossary/intro.rst
  invoking-from-command-line/intro.rst
  mixing/intro.rst
  mixing/overview.rst
  mixing/routing.rst
  modulators/intro.rst
  playback-and-recording/bpm-and-time-signatures.rst
  playback-and-recording/intro.rst
  playback-and-recording/loop-points-and-markers.rst
  playback-and-recording/overview.rst
  playback-and-recording/recording.rst
  playback-and-recording/transport-controls.rst
  plugins-files/intro.rst
  plugins-files/audio-midi-files/file-browser.rst
  plugins-files/audio-midi-files/intro.rst
  plugins-files/audio-midi-files/overview.rst
  plugins-files/plugins/inspector-page.rst
  plugins-files/plugins/intro.rst
  plugins-files/plugins/plugin-browser.rst
  plugins-files/plugins/plugin-info.rst
  plugins-files/plugins/plugin-window.rst
  plugins-files/plugins/scanning.rst
  projects/intro.rst
  projects/project-structure.rst
  projects/saving-loading.rst
  routing/connections.rst
  routing/intro.rst
  routing/monitor-section.rst
  routing/ports.rst
  scripting/intro.rst
  scripting/overview.rst
  theming/intro.rst
  theming/overview.rst
  tracks/automation.rst
  tracks/inspector-page.rst
  tracks/intro.rst
  tracks/track-types.rst
  tracks/creating-tracks.rst
  tracks/overview.rst
  tracks/track-lanes.rst
  tracks/track-operations.rst
  tracks/tracklist.rst
  zrythm-interface/bottom-panel.rst
  zrythm-interface/intro.rst
  zrythm-interface/left-panel.rst
  zrythm-interface/main-panel.rst
  zrythm-interface/main-toolbar.rst
  zrythm-interface/overview.rst
  zrythm-interface/right-panel.rst
  zrythm-interface/transport-bar.rst
)

set(built_rst_files)
foreach(src_rst_file ${src_rst_files})
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${src_rst_file}
    ${CMAKE_CURRENT_BINARY_DIR}/${src_rst_file}
    @ONLY
  )
  list(APPEND built_rst_files ${CMAKE_CURRENT_BINARY_DIR}/${src_rst_file})
endforeach()

add_subdirectory(_static)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/conf.py
  @ONLY
)

# Create targets for each lang/format combination
set(HTML_MANUALS)
set(SINGLEHTML_MANUALS)
foreach(LANG ${locales_list})
  foreach(FORMAT devhelp html singlehtml latex epub)
    set(NAME ${FORMAT}-manual-${LANG})
    set(OUTPUT_DIR ${SPHINX_BUILDDIR}/${LANG}/${FORMAT})
    
    if(FORMAT STREQUAL "html" OR FORMAT STREQUAL "singlehtml")
      add_custom_target(${NAME}
        BYPRODUCTS ${OUTPUT_DIR}
        # COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
        COMMAND ${SPHINX_BUILD_EXECUTABLE}
          ${SPHINX_BUILD_OPTS}
          -D "language=${LANG}"
          -b "${FORMAT}" "${CMAKE_CURRENT_BINARY_DIR}"
          "${OUTPUT_DIR}"
        DEPENDS custom_css_target copy_locales_targets
        SOURCES ${built_rst_files} ${HTML_TEMPLATES} ${CMAKE_CURRENT_BINARY_DIR}/conf.py
      )
      
      if(FORMAT STREQUAL "html")
        list(APPEND HTML_MANUALS ${NAME})
      elseif(FORMAT STREQUAL "singlehtml")
        list(APPEND SINGLEHTML_MANUALS ${NAME})
      endif()

      if(ZRYTHM_USER_MANUAL)
        install(DIRECTORY ${OUTPUT_DIR} DESTINATION ${CMAKE_INSTALL_DOCDIR})
      endif()
    endif()
  endforeach()
endforeach()

return() # rest TODO

# Link check
add_custom_target(sphinx_linkcheck
  COMMAND ${SPHINX_BUILD_EXECUTABLE} -b linkcheck
    ${SPHINX_BUILD_OPTS}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${SPHINX_BUILDDIR}/linkcheck
  DEPENDS ${HTML_MANUALS}
)

# Generate POT files
add_custom_target(manual_gettext_gen_pot
  COMMAND ${SPHINX_BUILD_EXECUTABLE}
    -M gettext
    ${CMAKE_CURRENT_BINARY_DIR}
    ${SPHINX_BUILDDIR}
  DEPENDS ${built_rst_files}
)

# Update PO files
foreach(LANG ${locales_list})
  if(NOT ${LANG} STREQUAL "en")
    set(PO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/locale/${LANG}/LC_MESSAGES/zrythm-manual.po)
    
    add_custom_target(${LANG}-intl-update
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/locale/${LANG}/LC_MESSAGES
      COMMAND ${MSGUNIQ_EXECUTABLE} --use-first --output=${PO_FILE} ${PO_FILE} || true
      COMMAND ${MSGATTRIB_EXECUTABLE} --clear-obsolete --output=${PO_FILE} ${PO_FILE} || true
      COMMAND ${SPHINX_INTL_EXECUTABLE} update -p
        ${SPHINX_BUILDDIR}/gettext
        -l ${LANG}
        -d ${CMAKE_CURRENT_SOURCE_DIR}/locale
      DEPENDS manual_gettext_gen_pot
    )
    
    list(APPEND SPHINX_INTL_UPDATE_TARGETS ${LANG}-intl-update)
  endif()
endforeach()

# Post-process localization
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/post_process_po.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/post_process_po.py
  @ONLY
)

add_custom_target(manual_gettext_post
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/post_process_po.py
  DEPENDS ${SPHINX_INTL_UPDATE_TARGETS}
)

# Bundle manual
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/bundle_manual.sh.in
  ${CMAKE_CURRENT_BINARY_DIR}/bundle_manual.sh
  @ONLY
)

add_custom_target(manual_bundle
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/bundle_manual.sh
    "${locales_str}"
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/_rendered
    ${SPHINX_BUILDDIR}
    ${CMAKE_CURRENT_BINARY_DIR}/bundled_manual.zip
  DEPENDS manual_gettext_post ${HTML_MANUALS} ${SINGLEHTML_MANUALS}
)

add_custom_target(manual_gettext
  DEPENDS manual_gettext_post
)

add_custom_target(manual_help
  COMMAND ${SPHINX_BUILD_EXECUTABLE} -M help
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
