# SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# =============================================================================
# Resources & QML
# =============================================================================

qt_add_executable(zrythm WIN32 MACOSX_BUNDLE)

set(_build_datetime "")
if(APPLE)
  execute_process(
    COMMAND date "+%Y.%m.%d.%H%M"
    OUTPUT_VARIABLE _build_datetime
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()

set_target_properties(zrythm PROPERTIES
  # note: separate variables by \n
  VS_DEBUGGER_ENVIRONMENT "ZRYTHM_SAMPLES_PATH=${CMAKE_SOURCE_DIR}/data/samples\nG_DEBUG=fatal_criticals"
  # note: separate variables by ;
  XCODE_SCHEME_ENVIRONMENT "ZRYTHM_SAMPLES_PATH=${CMAKE_SOURCE_DIR}/data/samples;G_DEBUG=fatal_criticals"
  MACOSX_BUNDLE_BUNDLE_NAME "Zrythm"
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/data/platform/macos/Info.plist.in"
  MACOSX_BUNDLE_GUI_IDENTIFIER "${bundle_identifier}"
  MACOSX_BUNDLE_ICON_FILE "${ZRYTHM_ICNS_FILE_NAME}"
  MACOSX_BUNDLE_BUNDLE_VERSION "${_build_datetime}" # build identifier
  MACOSX_BUNDLE_SHORT_VERSION_STRING "${ZRYTHM_VERSION_MAJOR}.${ZRYTHM_VERSION_MINOR}.${ZRYTHM_VERSION_PATCH}" # actual x.x.x version (no extra numbers allowed)
  MACOSX_BUNDLE_COPYRIGHT "© ${copyright_years} ${copyright_name}"
  XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_SOURCE_DIR}/data/platform/macos/Zrythm.entitlements"
  MACOSX_BUNDLE TRUE
  WIN32_EXECUTABLE TRUE
  UNITY_BUILD OFF # unity build is not supported by auto-generated Qt files
)

if(WIN32)
  # Copy all required DLLs and resources into the binary output directory
  # (needed to debug or run the test suite)
  add_custom_command(TARGET zrythm POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:zrythm> $<TARGET_RUNTIME_DLLS:zrythm>
    COMMAND "${WINDEPLOYQT_EXECUTABLE}"
      --qmldir "${CMAKE_BINARY_DIR}"
      --pdb --force
      -qml -widgets -quick -quickwidgets -test
      $<TARGET_FILE:zrythm>
    COMMAND_EXPAND_LISTS
  )
elseif(XCODE)
  # Ad-hoc sign (error that these files are not signed otherwise)
  add_custom_command(TARGET zrythm POST_BUILD
    COMMAND codesign -f -s - "$<TARGET_BUNDLE_CONTENT_DIR:zrythm>/MacOS/zrythm_qt.part.conf"
    COMMAND codesign -f -s - "$<TARGET_BUNDLE_CONTENT_DIR:zrythm>/MacOS/qt.conf"
    COMMENT "Signing zrythm.app for Xcode POST_BUILD"
  )
endif()

# add_subdirectory(resources)

# Generate language map for config.js
set(language_map_entries)
foreach(lang_pair ${language_mappings})
  string(REPLACE "/" ";" lang_list ${lang_pair})
  list(GET lang_list 0 lang_code)
  list(GET lang_list 1 lang_name)
  string(APPEND language_map_entries "    \"${lang_code}\": \"${lang_name}\",\n")
endforeach()
set(LANGUAGE_MAP_STRING "${language_map_entries}")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/js/config.js.in"
  "${CMAKE_CURRENT_BINARY_DIR}/js/config.js"
)

set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/js/config.js"
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  PROPERTIES QT_RESOURCE_ALIAS "config.js"
  )
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/resources/qtquickcontrols2.conf"
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  PROPERTIES QT_RESOURCE_ALIAS "qtquickcontrols2.conf"
  )

# Icons
set(icon_dirs
  arena gnome-builder gnome-icon-library fork-awesome font-awesome
  fluentui jam-icons box-icons iconpark iconoir material-design
  untitled-ui css.gg codicons lucide zrythm-dark
)

set(icon_files)
foreach(dir ${icon_dirs})
  file(GLOB_RECURSE icons "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/${dir}/*.svg" "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/${dir}/*.png")
  foreach(icon ${icons})
    get_filename_component(icon_name ${icon} NAME)
    set(icon_alias "icons/${dir}/${icon_name}")
    set_source_files_properties(${icon}
      DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      PROPERTIES QT_RESOURCE_ALIAS ${icon_alias}
      )
    list(APPEND icon_files ${icon})
  endforeach()
endforeach()

if(OS_GNU)
  # Install app icon
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/zrythm-dark/scalable/apps/zrythm.svg"
    RENAME "org.zrythm.Zrythm.svg"
    DESTINATION "share/icons/hicolor/scalable/apps"
    COMPONENT Runtime
  )
endif()

# font files
set(font_filepaths
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/NotoSans/NotoSans-VariableFont_wdth_wght.ttf
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/NotoSans/NotoSans-Italic-VariableFont_wdth_wght.ttf
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/Noto_Sans_Arabic/NotoSansArabic-VariableFont_wdth_wght.ttf
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/Noto_Sans_Hebrew/NotoSansHebrew-VariableFont_wdth_wght.ttf
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/Noto_Sans_Thai/NotoSansThai-VariableFont_wdth_wght.ttf
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/Noto_Sans_KR/NotoSansKR-VariableFont_wght.ttf
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/Noto_Sans_SC/NotoSansSC-VariableFont_wght.ttf
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/Noto_Sans_TC/NotoSansTC-VariableFont_wght.ttf
)
set(font_files)
foreach(font_filepath ${font_filepaths})
  get_filename_component(font_name ${font_filepath} NAME)
  set_source_files_properties(${font_filepath}
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTIES QT_RESOURCE_ALIAS "fonts/${font_name}"
    )
  list(APPEND font_files ${font_filepath})
endforeach()

set(rel_qml_file_paths
  components/basic/EditableValueDisplay.qml
  components/basic/LinkedButtons.qml
  components/basic/MuteButton.qml
  components/basic/PlaceholderPage.qml
  components/basic/RecordButton.qml
  components/basic/RotatedLabel.qml
  components/basic/SoloButton.qml
  components/basic/ZrythmActionRow.qml
  components/basic/ZrythmAlertDialog.qml
  components/basic/ZrythmFilePicker.qml
  components/basic/ZrythmPreferencesPage.qml
  components/basic/ZrythmResizablePanel.qml
  components/basic/SplitButton.qml
  components/basic/ZrythmToolBar.qml
  components/arranger/AudioRegionView.qml
  components/arranger/AutomationRegionView.qml
  components/arranger/Arranger.qml
  components/arranger/ArrangerObjectBaseView.qml
  components/arranger/ChordRegionView.qml
  components/arranger/MarkerView.qml
  components/arranger/MidiArranger.qml
  components/arranger/MidiNoteView.qml
  components/arranger/MidiRegionView.qml
  components/arranger/RegionBaseView.qml
  components/arranger/ScaleObjectView.qml
  components/arranger/Timeline.qml
  components/arranger/VelocityArranger.qml
  components/editors/AudioEditorPane.qml
  components/editors/AutomationEditorPane.qml
  components/editors/ClipEditorGrid.qml
  components/editors/MidiEditorPane.qml
  components/editors/ChordEditorPane.qml
  components/editors/PianoRollKeys.qml
  components/AutomationTracksListView.qml
  components/MainToolbar.qml
  components/BottomDock.qml
  components/CenterDock.qml
  components/LeftDock.qml
  components/MainMenuBar.qml
  components/Meter.qml
  components/PlayheadTriangle.qml
  components/PluginBrowserPage.qml
  components/RightDock.qml
  components/Ruler.qml
  components/ToolBox.qml
  components/Tracklist.qml
  components/TracklistHeader.qml
  components/TrackView.qml
  components/TransportControls.qml
  components/UndoSplitButton.qml
  Greeter.qml
  DemoView.qml
  utils/ArrangerObjectHelpers.qml
  views/ProjectWindow.qml
)
set(qml_file_paths)
foreach(qml_file_path ${rel_qml_file_paths})
  set(qml_abs_file_path "${CMAKE_CURRENT_SOURCE_DIR}/qml/${qml_file_path}")
  set_source_files_properties(${qml_abs_file_path}
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTIES QT_RESOURCE_ALIAS "${qml_file_path}"
    )
  list(APPEND qml_file_paths ${qml_abs_file_path})
endforeach()

# Singleton types defined in QML need to have this
# see https://doc.qt.io/qt-6/cmake-source-file-property-qt-qml-singleton-type.html
set_source_files_properties(
  ${CMAKE_CURRENT_SOURCE_DIR}/qml/utils/ArrangerObjectHelpers.qml
  PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

add_subdirectory(qml)

add_subdirectory(backend)
# temporarily keep old dsp classes here and slowly drop non-dsp dependencies
# and move to src/dsp
add_subdirectory(dsp)

target_include_directories(zrythm
  PUBLIC
    # C++ headers in SOURCES below must be available as toplevel includes
    # https://bugreports.qt.io/browse/QTBUG-87221
    ${CMAKE_CURRENT_SOURCE_DIR}/backend
    ${CMAKE_CURRENT_SOURCE_DIR}/qquick
)

qt_add_qml_module(zrythm
  URI Zrythm
  VERSION 1.0
  IMPORTS
    TARGET zrythm_dsp_lib
    TARGET zrythm_arrangement_lib
    TARGET zrythm_gui_lib
    TARGET zrythm_style
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Zrythm
  QML_FILES
    ${qml_file_paths}
    ${CMAKE_CURRENT_BINARY_DIR}/js/config.js
  RESOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/qtquickcontrols2.conf
    ${icon_files}
    ${font_files}

  # compile qml files listed in QML_FILES to C++ using qmltc
  # (currently cannot extend QtQuick Controls directly)
  # ENABLE_TYPE_COMPILER
)
