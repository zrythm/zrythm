# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# ==============================================================================================
# Resources & QML
# ==============================================================================================

qt_add_executable(zrythm WIN32 MACOSX_BUNDLE)

# add_subdirectory(resources)

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
  untitled-ui css.gg codicons lucide
)

set(icon_files)
foreach(dir ${icon_dirs})
  file(GLOB icons "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/${dir}/*.svg" "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/${dir}/*.png")
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

# add zrythm icons to resources
set(zrythm_logos zrythm.svg zrythm-monochrome.svg)
foreach(zlogo ${zrythm_logos})
  set(zlogo_path "${CMAKE_SOURCE_DIR}/data/icon-themes/zrythm-dark/scalable/apps/${zlogo}")
  set_source_files_properties("${zlogo_path}"
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTIES QT_RESOURCE_ALIAS "icons/${zlogo}"
    )
  list(APPEND icon_files ${zlogo_path})
endforeach()

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
  ProjectWindow.qml
  Greeter.qml
  components/ZrythmPlaceholderPage.qml
  components/ZrythmButton.qml
  components/ZrythmActionRow.qml
  components/ZrythmComboBox.qml
  components/ZrythmPreferencesPage.qml
  components/ZrythmFilePicker.qml
  components/ZrythmProgressBar.qml
  components/ZrythmDialogButtonBox.qml
  components/ZrythmTextField.qml
  components/ZrythmAlertDialog.qml
  components/ZrythmToolButton.qml
  components/ZrythmMenuBar.qml
  components/ZrythmResizablePanel.qml
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

add_subdirectory(backend)

target_include_directories(zrythm
  PUBLIC
    # C++ headers in SOURCES below must be available as toplevel includes
    # https://bugreports.qt.io/browse/QTBUG-87221
    ${CMAKE_CURRENT_SOURCE_DIR}/backend
)

qt_add_qml_module(zrythm
  URI Zrythm
  VERSION 1.0
  RESOURCE_PREFIX /org.zrythm/imports
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Zrythm
  QML_FILES
    ${qml_file_paths}
    ${CMAKE_CURRENT_BINARY_DIR}/js/config.js
  RESOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/qtquickcontrols2.conf
    ${icon_files}
    ${font_files}
  SOURCES # for some reason singletons must be added here
    backend/global_state.h
    backend/global_state.cpp
    backend/recent_projects_model.h
    backend/recent_projects_model.cpp
    backend/project_info.h
    backend/project_info.cpp
    backend/project_templates_model.h
    backend/project_templates_model.cpp

  # compile qml files listed in QML_FILES to C++ using qmltc
  # (currently cannot extend QtQuick Controls directly)
  # ENABLE_TYPE_COMPILER
)