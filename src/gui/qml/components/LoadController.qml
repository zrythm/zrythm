// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtCore
import QtQuick
import QtQuick.Dialogs
import Zrythm

Item {
  id: root

  readonly property Action loadAction: Action {
    shortcut: StandardKey.Open
    text: qsTr("Open…")

    onTriggered: {
      root.loadFolderDialog.open();
    }
  }
  property FolderDialog loadFolderDialog: FolderDialog {
    title: qsTr("Open Project")
    options: FolderDialog.ShowDirsOnly
    selectedFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)

    onAccepted: {
      root.loadProgressDialog.resetValues();
      root.loadProgressDialog.open();
      root.loadFuture = GlobalState.application.projectManager.loadProject(QmlUtils.toPathString(selectedFolder));
    }
  }
  property QFutureQmlWrapper loadFuture
  property ProgressDialogWithFuture loadProgressDialog: ProgressDialogWithFuture {
    future: root.loadFuture
    labelText: qsTr("Loading project...")
  }

  function openLoadDialog() {
    root.loadFolderDialog.open();
  }
}
