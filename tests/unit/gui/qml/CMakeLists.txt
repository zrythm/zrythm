# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_executable(zrythm_gui_qml_unit_tests qml_test_main.cpp)

qt_add_qml_module(zrythm_gui_qml_unit_tests
  URI QmlTests
  VERSION 1.0
  IMPORTS
    TARGET zrythm_docking
    TARGET zrythm_qml_lib
)

target_link_libraries(zrythm_gui_qml_unit_tests PRIVATE
  Qt6::QuickTest
  zrythm_utils_lib
)

target_include_directories(zrythm_gui_qml_unit_tests PRIVATE
  "${CMAKE_SOURCE_DIR}/tests"
)

target_compile_definitions(zrythm_gui_qml_unit_tests PRIVATE
  QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

# Qt Quick Test always shows a window and waits for it to become exposed.
# On headless CI (Windows Session 0, etc.) the expose event never arrives
# and the test hangs until the ctest timeout. The offscreen QPA plugin
# treats windows as exposed immediately.
# See https://doc.qt.io/qt-6/qtquicktest-index.html
add_test(
  NAME zrythm_gui_qml_unit_tests
  COMMAND zrythm_gui_qml_unit_tests -platform offscreen
)

if(WIN32)
  add_dependencies(zrythm_gui_qml_unit_tests zrythm)

  add_custom_command(TARGET zrythm_gui_qml_unit_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:zrythm_gui_qml_unit_tests> $<TARGET_FILE_DIR:zrythm_gui_qml_unit_tests>
    COMMAND_EXPAND_LISTS
  )

  # DLLs are copied next to the exe (above), so Qt resolves plugin and QML
  # paths relative to that directory rather than the real Qt installation.
  # Point Qt back at the installation so it finds the offscreen plugin and
  # Qt-provided QML modules (e.g. QtTest) that `zrythm` doesn't deploy.
  # QT_QUICK_BACKEND=software avoids GPU/D3D access in headless environments.
  set_tests_properties(zrythm_gui_qml_unit_tests PROPERTIES
    ENVIRONMENT "QT_PLUGIN_PATH=${QT6_INSTALL_PREFIX}/${QT6_INSTALL_PLUGINS};QML_IMPORT_PATH=${QT6_INSTALL_PREFIX}/${QT6_INSTALL_QML};QT_QUICK_BACKEND=software"
  )
endif()
