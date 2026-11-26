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

    onTriggered: root.save()
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

  /**
   * A save or a Save As ended. @p saved is true when the project was
   * written to disk.
   */
  signal saveEnded(bool saved)

  /**
   * Saves the project to its directory. Returns false when the project
   * has no directory: nothing is saved then and saveEnded() is not
   * emitted.
   */
  function save(): bool {
    if (!root.session || root.session.projectDirectory === "")
      return false;

    root.saveProgressDialog.resetValues();
    root.saveProgressDialog.open();
    root.saveFuture = root.session.save();
    return true;
  }

  Connections {
    function onCanceled() {
      root.saveEnded(false);
    }

    function onFailed(errorString: string) {
      root.notificationCenter.postCritical(qsTr("Project Save Failed"), errorString);
      root.saveEnded(false);
    }

    function onSucceeded() {
      // An empty result means the operation was refused before starting
      const savedPath = root.saveFuture.resultVar();
      const saved = Boolean(savedPath);
      if (saved)
        GlobalState.application.projectManager.recentProjects.addRecentProject(savedPath);
      root.saveEnded(saved);
    }

    target: root.saveFuture
  }
}
