# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense */

# ==============================================================================================
# Resources & QML
# ==============================================================================================

# add_subdirectory(resources)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/js/config.js.in"
  "${CMAKE_CURRENT_BINARY_DIR}/js/config.js"
)

set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/js/config.js" 
  TARGET_DIRECTORY zrythm  
  PROPERTIES QT_RESOURCE_ALIAS "config.js"
  )
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/resources/qtquickcontrols2.conf"
  TARGET_DIRECTORY zrythm
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
      TARGET_DIRECTORY zrythm
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
    TARGET_DIRECTORY zrythm
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
    TARGET_DIRECTORY zrythm  
    PROPERTIES QT_RESOURCE_ALIAS "fonts/${font_name}"
    )
  list(APPEND font_files ${font_filepath})
endforeach()

set(rel_qml_file_paths
  main_window.qml
  greeter.qml
  components/PlaceholderPage.qml
  components/PlainButton.qml
  components/ActionRow.qml
  components/ComboBoxText.qml
  components/PreferencesPage.qml
  components/FilePicker.qml
  components/CustomProgressBar.qml
)
set(qml_file_paths)
foreach(qml_file_path ${rel_qml_file_paths})
  get_filename_component(qml_filename ${qml_file_path} NAME)
  set(qml_abs_file_path "${CMAKE_CURRENT_SOURCE_DIR}/qml/${qml_file_path}")
  set_source_files_properties(${qml_abs_file_path}
    TARGET_DIRECTORY zrythm
    PROPERTIES QT_RESOURCE_ALIAS "${qml_file_path}"
    )
  list(APPEND qml_file_paths ${qml_abs_file_path})
endforeach()

set(qml_file_paths ${qml_file_paths} PARENT_SCOPE)
set(icon_files ${icon_files} PARENT_SCOPE)
set(font_files ${font_files} PARENT_SCOPE)

add_subdirectory(backend)
add_subdirectory(widgets)