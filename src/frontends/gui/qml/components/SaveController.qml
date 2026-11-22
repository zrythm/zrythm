// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
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
      root.saveAsDialog.open();
    }
  }
  property SaveAsDialog saveAsDialog: SaveAsDialog {
    initialLocation: root.session && root.session.projectDirectory !== ""
      ? QmlUtils.parentFolder(QmlUtils.localFileToQUrl(root.session.projectDirectory))
      : QmlUtils.localFileToQUrl(GlobalState.application.appSettings.newProjectDirectory)
    initialName: root.session ? root.session.title : ""
    projectManager: GlobalState.application.projectManager

    onSaveAccepted: (parentFolder, name, overwrite) => {
      root.saveProgressDialog.resetValues();
      root.saveProgressDialog.open();
      root.saveFuture = root.session.saveAs(parentFolder, name, overwrite);
    }
  }
  property QFutureQmlWrapper saveFuture
  property ProgressDialogWithFuture saveProgressDialog: ProgressDialogWithFuture {
    future: root.saveFuture
    labelText: qsTr("Saving project...")
  }
  required property NotificationCenter notificationCenter
  required property ProjectSession session

  Connections {
    function onFailed(errorString: string) {
      root.notificationCenter.postCritical(qsTr("Project Save Failed"), errorString);
    }

    function onSucceeded() {
      // An empty result means the operation was refused before starting
      const savedPath = root.saveFuture.resultVar();
      if (savedPath && savedPath !== "") {
        GlobalState.application.projectManager.recentProjects.addRecentProject(savedPath);
      }
    }

    target: root.saveFuture
  }
}
