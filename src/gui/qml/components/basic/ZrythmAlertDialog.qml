// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls.Basic

Dialog {
    id: alertDialog
    title: "Alert"
    modal: true
    standardButtons: Dialog.Ok

    property alias alertTitle: titleText.text
    property alias alertMessage: messageText.text

    Column {
        spacing: 10
        Text {
            id: titleText
            font.bold: true
            font.pixelSize: 16
        }
        Text {
            id: messageText
            wrapMode: Text.Wrap
            width: parent.width
        }
    }
}