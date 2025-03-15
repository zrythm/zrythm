// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property alias text: rotatedText.text
    property alias rotation: rotatedText.rotation
    property alias font: rotatedText.font

    implicitWidth: textMetrics.height
    implicitHeight: textMetrics.width

    TextMetrics {
        id: textMetrics

        text: rotatedText.text
        font: rotatedText.font
    }

    Label {
        id: rotatedText

        anchors.fill: parent
        text: "Test Label"
        rotation: 270
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

}
