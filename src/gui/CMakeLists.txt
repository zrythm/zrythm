# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# ==============================================================================================
# Resources & QML
# ==============================================================================================

# set(MACOSX_BUNDLE_GUI_IDENTIFIER ${bundle_identifier})
qt_add_executable(zrythm WIN32 MACOSX_BUNDLE)

set_target_properties(zrythm PROPERTIES
  # note: separate variables by \n
  VS_DEBUGGER_ENVIRONMENT "ZRYTHM_SAMPLES_PATH=${CMAKE_SOURCE_DIR}/data/samples\nG_DEBUG=fatal_criticals"
  # note: separate variables by ;
  XCODE_SCHEME_ENVIRONMENT "ZRYTHM_SAMPLES_PATH=${CMAKE_SOURCE_DIR}/data/samples;G_DEBUG=fatal_criticals"
  MACOSX_BUNDLE_GUI_IDENTIFIER "${bundle_identifier}"
  MACOSX_BUNDLE_BUNDLE_VERSION ${ZRYTHM_VERSION_STRING_FULL}
  MACOSX_BUNDLE_SHORT_VERSION_STRING ${ZRYTHM_VERSION}
  MACOSX_BUNDLE TRUE
  WIN32_EXECUTABLE TRUE
)

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

# Install app icon
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/zrythm-dark/scalable/apps/zrythm.svg"
  RENAME "org.zrythm.Zrythm.svg"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps")


# font files
set(font_filepaths
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/Inter-4.0/InterVariable.ttf
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/Inter-4.0/InterVariable-Italic.ttf
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/DSEG14-Classic-MINI/DSEG14ClassicMini-Regular.ttf
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/DSEG14-Classic-MINI/DSEG14ClassicMini-Bold.ttf
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/DSEG14-Classic-MINI/DSEG14ClassicMini-Light.ttf
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/DSEG14-Classic-MINI/DSEG14ClassicMini-Italic.ttf
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/DSEG14-Classic-MINI/DSEG14ClassicMini-BoldItalic.ttf
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts/DSEG14-Classic-MINI/DSEG14ClassicMini-LightItalic.ttf
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
  components/arranger/AudioRegion.qml
  components/arranger/AutomationRegion.qml
  components/arranger/Arranger.qml
  components/arranger/ArrangerObjectBase.qml
  components/arranger/ChordRegion.qml
  components/arranger/Marker.qml
  components/arranger/MidiArranger.qml
  components/arranger/MidiNote.qml
  components/arranger/MidiRegion.qml
  components/arranger/RegionBase.qml
  components/arranger/ScaleObject.qml
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
)

qt_add_qml_module(zrythm
  URI Zrythm
  VERSION 1.0
  IMPORTS ZrythmStyle/1.0
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Zrythm
  QML_FILES
    ${qml_file_paths}
    ${CMAKE_CURRENT_BINARY_DIR}/js/config.js
  RESOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/qtquickcontrols2.conf
    ${icon_files}
    ${font_files}
  # objects that need to be creatable and singletons/enums must be added here
  SOURCES
    backend/action_controller.h
    backend/action_controller.cpp
    backend/automation_tracklist_proxy_model.h
    backend/automation_tracklist_proxy_model.cpp
    backend/cursor_manager.h
    backend/cursor_manager.cpp
    backend/global_state.h
    backend/global_state.cpp
    backend/meter.h
    backend/meter.cpp
    backend/position_proxy.h
    backend/position_proxy.cpp
    backend/project_info.h
    backend/project_info.cpp
    backend/project_templates_model.h
    backend/project_templates_model.cpp
    backend/qml_utils.h
    backend/qml_utils.cpp
    backend/realtime_property.h
    backend/realtime_updater.h
    backend/realtime_updater.cpp
    backend/recent_projects_model.h
    backend/recent_projects_model.cpp
    backend/resource_manager.h
    backend/resource_manager.cpp
    backend/tool.h
    backend/tool.cpp
    backend/track_filter_proxy_model.h
    backend/track_filter_proxy_model.cpp
    backend/translation_manager.h
    backend/translation_manager.cpp
    backend/zrythm_application.h
    backend/zrythm_application.cpp

  # compile qml files listed in QML_FILES to C++ using qmltc
  # (currently cannot extend QtQuick Controls directly)
  # ENABLE_TYPE_COMPILER
)

target_link_libraries(zrythm PRIVATE zrythm_styleplugin)
