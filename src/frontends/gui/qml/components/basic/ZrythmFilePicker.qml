// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

RowLayout {
  id: control

  property string buttonLabel: qsTr("Browse")
  property string defaultPath // optional path the reset button restores
  property string initialPath // optional path used as the initial value and the dialog's starting folder
  property bool pickFolder: true // whether to pick a folder or a file

  // Value chosen so far: the initial path until the user accepts a selection
  // in the dialog, then the last accepted selection
  property url selectedUrl: control.initialUrl

  readonly property url initialUrl: QmlUtils.localFileToQUrl(control.initialPath)

  TextField {
    id: textField

    readonly property string selectedPath: QmlUtils.toPathString(control.selectedUrl)

    Layout.fillWidth: true
    readOnly: true
    text: textField.selectedPath.length > 0 ? textField.selectedPath : qsTr("(no path selected)")
  }

  Button {
    id: btn

    text: control.buttonLabel

    onClicked: control.pickFolder ? folderDialog.open() : fileDialog.open()
  }

  Loader {
    id: resetButtonLoader

    // defaultPath is fixed at creation, so the button's existence is decided once
    active: false
    sourceComponent: resetButtonComponent

    Component.onCompleted: resetButtonLoader.active = control.defaultPath.length > 0
  }

  Component {
    id: resetButtonComponent

    Button {
      Accessible.name: qsTr("Reset to Default Location")
      display: AbstractButton.IconOnly
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "revert-symbolic.svg")

      onClicked: control.selectedUrl = QmlUtils.localFileToQUrl(control.defaultPath)

      ToolTip {
        text: qsTr("Reset to Default Location")
      }
    }
  }

  FolderDialog {
    id: folderDialog

    currentFolder: control.selectedUrl

    onAccepted: control.selectedUrl = folderDialog.selectedFolder
  }

  // Preselected file for the file dialog; only active when picking files
  Binding {
    property: "currentFile"
    target: fileDialog
    value: control.selectedUrl
    when: !control.pickFolder
  }

  FileDialog {
    id: fileDialog

    fileMode: FileDialog.OpenFile

    onAccepted: control.selectedUrl = fileDialog.selectedFile
  }
}
