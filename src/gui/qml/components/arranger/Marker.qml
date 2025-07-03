// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ArrangerObjectBase {
    id: root

    width: textMetrics.width + 2 * Style.buttonPadding
    height: textMetrics.height + 2 * Style.buttonPadding

    Rectangle {
        color: root.objectColor
        anchors.fill: parent
        radius: Style.toolButtonRadius
    }

    Text {
        id: nameText

        text: arrangerObject.name.name
        color: root.palette.text
        font: root.font
        padding: Style.buttonPadding
        anchors.centerIn: parent
    }

    TextMetrics {
        id: textMetrics

        text: nameText.text
        font: nameText.font
    }

}
