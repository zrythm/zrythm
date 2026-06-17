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
  # `zrythm` deploys the shared Qt runtime into this directory first; this
  # target only adds the QtTest bits it additionally needs (see below).
  add_dependencies(zrythm_gui_qml_unit_tests zrythm)

  # Deploy `Qt6::QuickTest`, the `QtTest` QML module (`import QtTest`) and the
  # offscreen platform plugin (`-platform offscreen`) that `zrythm` does not
  # bring in. `copy_if_different` and the absence of `--force` keep this purely
  # additive -- shared files already deployed by `zrythm` are left untouched,
  # avoiding `windeployqt`'s `DeleteFile` failing on them with
  # "Cannot remove existing file ...".
  add_custom_command(TARGET zrythm_gui_qml_unit_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different -t $<TARGET_FILE_DIR:zrythm_gui_qml_unit_tests> $<TARGET_RUNTIME_DLLS:zrythm_gui_qml_unit_tests>
    COMMAND "${WINDEPLOYQT_EXECUTABLE}"
      --qmldir "${CMAKE_BINARY_DIR}"
      -qml -quick -test
      $<TARGET_FILE:zrythm_gui_qml_unit_tests>
    COMMAND_EXPAND_LISTS
  )
endif()
