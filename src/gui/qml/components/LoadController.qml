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
      root.loadFolderDialog.open();
    }
  }
  property FolderDialog loadFolderDialog: FolderDialog {
    currentFolder: QmlUtils.localFileToQUrl(GlobalState.application.appSettings.newProjectDirectory)
    options: FolderDialog.ShowDirsOnly
    title: qsTr("Open Project")

    onAccepted: {
      root.loadProject(QmlUtils.toPathString(selectedFolder));
    }
  }
  property QFutureQmlWrapper loadFuture
  property ProgressDialogWithFuture loadProgressDialog: ProgressDialogWithFuture {
    future: root.loadFuture
    labelText: qsTr("Loading project...")
  }

  function loadProject(path: string) {
    const future = GlobalState.application.projectManager.loadProject(path);
    if (!future)
      return;

    root.loadProgressDialog.resetValues();
    root.loadProgressDialog.open();
    root.loadFuture = future;
  }

  function openLoadDialog() {
    root.loadFolderDialog.open();
  }
}
