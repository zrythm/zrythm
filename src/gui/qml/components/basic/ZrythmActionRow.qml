// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ZrythmStyle 1.0

Item {
    id: control

    property string title: "aaaa"
    property string subtitle: "bbbb"
    default property alias content: suffixLayout.data

    implicitHeight: layout.implicitHeight
    Layout.fillWidth: true

    RowLayout {
        id: layout

        spacing: 12
        anchors.fill: parent

        ColumnLayout {
            spacing: 2

            Label {
                id: titleLabel

                text: control.title
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Label {
                id: subtitleLabel

                text: control.subtitle
                visible: text !== ""
                opacity: 0.7
                font.pixelSize: titleLabel.font.pixelSize * 0.9
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

        }

        RowLayout {
            id: suffixLayout

            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            Layout.fillWidth: true
            spacing: 8
        }

    }

}
