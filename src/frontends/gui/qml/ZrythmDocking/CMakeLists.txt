# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

qt_add_library(zrythm_docking STATIC)

qt_add_qml_module(zrythm_docking
  URI ZrythmDocking
  VERSION 1.0
  IMPORTS
    QtQuick
    QtQuick.Controls
  QML_FILES
    DockFrame.qml
    DockDropOverlay.qml
    DockingController.qml
    DockingLayout.js
    DockNode.qml
    DockPanel.qml
    DockWindow.qml
    DockWorkspace.qml
    EmptyDockArea.qml
)

target_link_libraries(zrythm_docking PUBLIC Qt6::Gui Qt6::Quick Qt6::QuickControls2)
