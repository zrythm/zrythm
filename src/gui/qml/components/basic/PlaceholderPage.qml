// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Layouts
import ZrythmStyle 1.0

ColumnLayout {
    id: root

    property alias icon: placeholderIcon
    property alias title: titleLabel.text
    property alias description: descriptionLabel.text
    property alias action: actionButton.action
    property alias buttonText: actionButton.text

    spacing: 10
    width: Math.min(parent.width - 40, parent.width * 0.8)

    IconImage {
        id: placeholderIcon

        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: 128
        Layout.preferredHeight: 128
        Layout.bottomMargin: 16
        fillMode: Image.PreserveAspectFit
        visible: status === Image.Ready
        // these ensure SVGs are scaled and not stretched
        sourceSize.width: width
        sourceSize.height: height
        color: root.palette.buttonText
    }

    Label {
        id: titleLabel

        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        font.pointSize: 20
        font.bold: true
    }

    Label {
        id: descriptionLabel

        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        font.pointSize: 10.5
        text: ""
        visible: text !== ""
        onLinkActivated: (link) => {
            console.log("Link clicked:", link);
            Qt.openUrlExternally(link);
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
        }

    }

    Button {
        id: actionButton

        highlighted: true
        Layout.topMargin: 10
        Layout.alignment: Qt.AlignHCenter
        visible: action || text
    }

}
