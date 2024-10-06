// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ScrollView {
    id: control

    property string title: ""
    default property alias content: contentLayout.children

    clip: true
    contentWidth: availableWidth
    Layout.fillWidth: true

    ColumnLayout {
        width: control.width
        spacing: 0

        Label {
            id: pageTitle

            text: control.title
            font.pointSize: 16
            font.weight: Font.Bold
            padding: 16
            Layout.fillWidth: true
        }

        ColumnLayout {
            id: contentLayout

            Layout.fillWidth: true
            Layout.margins: 16
            spacing: 16
        }

    }

}
