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

if(WIN32)
  # Deploy Qt DLLs/plugins/QML modules needed by this test executable. The
  # main `zrythm` target's post-build step does not cover QtQuickTest/QtTest
  # (which only this test links), so without this the binary fails to start
  # with STATUS_DLL_NOT_FOUND (0xc0000135).
  add_custom_command(TARGET zrythm_gui_qml_unit_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:zrythm_gui_qml_unit_tests> $<TARGET_RUNTIME_DLLS:zrythm_gui_qml_unit_tests>
    COMMAND "${WINDEPLOYQT_EXECUTABLE}"
      --qmldir "${CMAKE_BINARY_DIR}"
      --force
      -qml -quick -test
      $<TARGET_FILE:zrythm_gui_qml_unit_tests>
    COMMAND_EXPAND_LISTS
  )
endif()
