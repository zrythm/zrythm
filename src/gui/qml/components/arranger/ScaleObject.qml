// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ArrangerObjectBase {
    id: root

    height: textMetrics.height + 2 * Style.buttonPadding
    width: textMetrics.width + 2 * Style.buttonPadding

    Rectangle {
        color: Style.adjustColorForHoverOrVisualFocusOrDown(track.color, root.hovered, root.visualFocus, root.down)
        anchors.fill: parent
        radius: Style.toolButtonRadius
    }

    Text {
        id: nameText

        text: arrangerObject.name
        font: root.font
        color: root.palette.text
        padding: Style.buttonPadding
    }

    TextMetrics {
        id: textMetrics

        text: nameText.text
        font: nameText.font
    }

}
