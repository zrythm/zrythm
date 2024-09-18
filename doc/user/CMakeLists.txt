# SPDX-FileCopyrightText: © 2019-2020, 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

if(NOT ZRYTHM_USER_MANUAL)
  return()
endif()

# TODO
return()

find_program(SPHINX_BUILD_EXECUTABLE NAMES "sphinx-build" "sphinx-build-3" REQUIRED)
find_program(SPHINX_INTL_EXECUTABLE "sphinx-intl" REQUIRED)
find_program(SASS_EXECUTABLE "sass" REQUIRED)

if(NOT SPHINX_BUILD_EXECUTABLE OR NOT SPHINX_INTL_EXECUTABLE OR NOT SASS_EXECUTABLE)
  message(WARNING "sphinx-build/sphinx-intl/sass not found - user manual cannot be built")
  return()
endif()

# For msgattrib, msguniq, and msggrep, you might need to construct the paths
# based on the Gettext installation directory
get_filename_component(GETTEXT_BIN_DIR ${Gettext_MSGMERGE_EXECUTABLE} DIRECTORY)

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

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/run_sphinx_build.sh.in
  ${CMAKE_CURRENT_BINARY_DIR}/run_sphinx_build.sh
  @ONLY
)

add_subdirectory(locale)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/index.rst.in
  ${CMAKE_CURRENT_BINARY_DIR}/index.rst
  @ONLY
)
set(RST_FILES ${CMAKE_CURRENT_BINARY_DIR}/index.rst)

add_subdirectory(_static)
add_subdirectory(appendix)
add_subdirectory(chords-and-scales)
add_subdirectory(configuration)
add_subdirectory(contributing)
add_subdirectory(credits)
add_subdirectory(editing)
add_subdirectory(exporting)
add_subdirectory(getting-started)
add_subdirectory(glossary)
add_subdirectory(invoking-from-command-line)
add_subdirectory(mixing)
add_subdirectory(modulators)
add_subdirectory(playback-and-recording)
add_subdirectory(plugins-files)
add_subdirectory(projects)
add_subdirectory(routing)
add_subdirectory(scripting)
add_subdirectory(theming)
add_subdirectory(tracks)
add_subdirectory(zrythm-interface)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/conf.py
  @ONLY
)

# Create targets for each lang/format combination
set(HTML_MANUALS)
set(SINGLEHTML_MANUALS)
foreach(LANG ${LOCALES})
  foreach(FORMAT devhelp html singlehtml latex epub)
    set(NAME ${FORMAT}-manual-${LANG})
    set(OUTPUT_DIR ${SPHINX_BUILDDIR}/${LANG}/${FORMAT})
    
    add_custom_target(${NAME}
      COMMAND ${CMAKE_CURRENT_BINARY_DIR}/run_sphinx_build.sh
        ${SPHINX_BUILD_EXECUTABLE}
        "${SPHINX_BUILD_OPTS}"
        ${LANG} ${FORMAT} ${CMAKE_CURRENT_BINARY_DIR}
        ${OUTPUT_DIR}
      DEPENDS custom_css_target copy_locales_targets
    )

    if(FORMAT STREQUAL "html" OR FORMAT STREQUAL "singlehtml")
      add_custom_target(${NAME}_build
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/run_sphinx_build.sh
          ${SPHINX_BUILD_EXECUTABLE}
          "${SPHINX_BUILD_OPTS}"
          ${LANG} ${FORMAT} ${CMAKE_CURRENT_BINARY_DIR}
          ${OUTPUT_DIR}
        DEPENDS ${NAME}
        SOURCES ${RST_FILES} ${HTML_TEMPLATES} ${CMAKE_CURRENT_BINARY_DIR}/conf.py
      )
      
      if(FORMAT STREQUAL "html")
        list(APPEND HTML_MANUALS ${NAME}_build)
      elseif(FORMAT STREQUAL "singlehtml")
        list(APPEND SINGLEHTML_MANUALS ${NAME}_build)
      endif()

      if(ZRYTHM_USER_MANUAL)
        install(DIRECTORY ${OUTPUT_DIR} DESTINATION ${CMAKE_INSTALL_DOCDIR})
      endif()
    endif()
  endforeach()
endforeach()

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
  DEPENDS ${RST_FILES}
)

# Update PO files
foreach(LANG ${LOCALES})
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
    "${LOCALES}"
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
