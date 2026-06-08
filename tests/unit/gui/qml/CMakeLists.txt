# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_executable(zrythm_gui_qml_unit_tests qml_test_main.cpp)

qt_add_qml_module(zrythm_gui_qml_unit_tests
  URI QmlTests
  VERSION 1.0
  IMPORTS
    TARGET zrythm_qml_lib
)

target_link_libraries(zrythm_gui_qml_unit_tests PRIVATE
  Qt6::QuickTest
)

target_compile_definitions(zrythm_gui_qml_unit_tests PRIVATE
  QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

add_test(
  NAME zrythm_gui_qml_unit_tests
  COMMAND zrythm_gui_qml_unit_tests -platform offscreen
)
