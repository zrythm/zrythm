// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Dialogs
import Zrythm

Item {
  id: root

  readonly property Action saveAction: Action {
    shortcut: StandardKey.Save
    text: qsTr("Save")

    onTriggered: {
      if (root.session && root.session.projectDirectory !== "") {
        root.saveProgressDialog.resetValues();
        root.saveProgressDialog.open();
        root.saveFuture = root.session.save();
      }
    }
  }
  readonly property Action saveAsAction: Action {
    shortcut: StandardKey.SaveAs
    text: qsTr("Save As…")

    onTriggered: {
      root.saveAsFolderDialog.open();
    }
  }
  property FolderDialog saveAsFolderDialog: FolderDialog {
    title: qsTr("Save Project As")

    onAccepted: {
      root.saveProgressDialog.resetValues();
      root.saveProgressDialog.open();
      root.saveFuture = root.session.saveAs(QmlUtils.toPathString(selectedFolder));
    }
  }
  property QFutureQmlWrapper saveFuture
  property ProgressDialogWithFuture saveProgressDialog: ProgressDialogWithFuture {
    future: root.saveFuture
    labelText: qsTr("Saving project...")
  }
  required property ProjectSession session

  Connections {
    function onFinished() {
      const savedPath = root.saveFuture.resultVar;
      if (savedPath && savedPath !== "") {
        GlobalState.application.projectManager.recentProjects.addRecentProject(savedPath);
      }
    }

    target: root.saveFuture
  }
}
