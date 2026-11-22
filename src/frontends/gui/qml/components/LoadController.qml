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
      // loadProject() expects the project directory containing the
      // project.zpj file
      root.loadProject(QmlUtils.toPathString(QmlUtils.parentFolder(root.loadFileDialog.selectedFile)));
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
    root.loadFileDialog.open();
  }
}
