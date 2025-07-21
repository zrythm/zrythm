// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

RowLayout {
  id: control

  property string buttonLabel: qsTr("Browse")
  property string initialPath // optional path to use as initial value (auto-sets initialUrl)
  property url initialUrl // optional URL to use as initial value
  property bool pickFolder: true // whether to pick a folder or a file

  property url selectedUrl: pickFolder ? folderDialog.currentFolder : fileDialog.currentFile // to communicate the result to the parent

  Binding {
    property: "initialUrl"
    target: control
    value: QmlUtils.localFileToQUrl(control.initialPath)
    when: control.initialPath !== undefined
  }

  Binding {
    property: "currentFile"
    target: fileDialog
    value: control.initialUrl
    when: !control.pickFolder && control.initialUrl !== undefined
  }

  Binding {
    property: "currentFolder"
    target: folderDialog
    value: control.initialUrl
    when: control.pickFolder && control.initialUrl !== undefined
  }

  TextField {
    id: textField

    readonly property url currentUrl: control.pickFolder ? folderDialog.currentFolder : fileDialog.currentFile

    Layout.fillWidth: true
    readOnly: true
    text: currentUrl ? QmlUtils.toPathString(currentUrl) : "(no path selected)"
  }

  Button {
    id: btn

    text: control.buttonLabel

    onClicked: control.pickFolder ? folderDialog.open() : fileDialog.open()
  }

  FolderDialog {
    id: folderDialog

  }

  FileDialog {
    id: fileDialog

    fileMode: FileDialog.OpenFile
  }
}
