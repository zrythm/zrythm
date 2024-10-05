// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts

RowLayout {
    id: control

    property alias text: btn.text
    property alias selectedPath: textField.text
    property bool pickFolder: true

    TextField {
        id: textField

        Layout.fillWidth: true
        text: pickFolder ? folderDialog.selectedFolder : fileDialog.selectedFile
        readOnly: true
    }

    PlainButton {
        id: btn

        text: qsTr("Browse")
        onClicked: {
            if (pickFolder)
                folderDialog.open();
            else
                fileDialog.open();
        }
    }

    FolderDialog {
        id: folderDialog

        onAccepted: {
            textField.text = selectedFolder;
        }
    }

    FileDialog {
        id: fileDialog

        fileMode: FileDialog.OpenFile
        onAccepted: {
            textField.text = selectedFile;
        }
    }

}
