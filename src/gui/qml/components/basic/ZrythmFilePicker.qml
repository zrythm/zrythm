// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import ZrythmStyle 1.0
import QtQuick.Dialogs
import QtQuick.Layouts
import Zrythm 1.0

RowLayout {
    id: control

    property string buttonLabel: qsTr("Browse")
    property string initialPath // optional path to use as initial value (auto-sets initialUrl)
    property url initialUrl // optional URL to use as initial value
    property url selectedUrl: pickFolder ? folderDialog.currentFolder : fileDialog.currentFile // to communicate the result to the parent
    property bool pickFolder: true // whether to pick a folder or a file

    Binding {
        target: control
        property: "initialUrl"
        value: QmlUtils.localFileToQUrl(control.initialPath)
        when: control.initialPath !== undefined
    }

    Binding {
        target: fileDialog
        property: "currentFile"
        value: control.initialUrl
        when: !control.pickFolder && control.initialUrl !== undefined
    }

    Binding {
        target: folderDialog
        property: "currentFolder"
        value: control.initialUrl
        when: control.pickFolder && control.initialUrl !== undefined
    }

    TextField {
        id: textField

        readonly property url currentUrl: control.pickFolder ? folderDialog.currentFolder : fileDialog.currentFile

        Layout.fillWidth: true
        text: currentUrl ? QmlUtils.toPathString(currentUrl) : "(no path selected)"
        readOnly: true
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
