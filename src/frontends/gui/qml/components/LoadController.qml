// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import Zrythm

Item {
  id: root

  readonly property Action loadAction: Action {
    shortcut: StandardKey.Open
    text: qsTr("Open…")

    onTriggered: {
      root.loadFileDialog.open();
    }
  }
  property FileDialog loadFileDialog: FileDialog {
    currentFolder: QmlUtils.localFileToQUrl(GlobalState.application.appSettings.newProjectDirectory)
    nameFilters: [qsTr("Zrythm projects (*.zpj)"), qsTr("All files (*)")]
    title: qsTr("Open Project")

    onAccepted: {
      root.requestProject(QmlUtils.toPathString(root.loadFileDialog.selectedFile));
    }
  }

  /**
   * Requests that the project at @p path be opened. The project opens
   * once no other project is open.
   */
  function requestProject(path: string) {
    GlobalState.application.setPendingProjectFile(path);
  }

  function openLoadDialog() {
    root.loadFileDialog.open();
  }
}
