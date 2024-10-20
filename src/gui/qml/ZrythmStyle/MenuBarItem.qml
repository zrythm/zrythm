// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import ZrythmStyle 1.0

T.MenuBarItem {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding, implicitIndicatorHeight + topPadding + bottomPadding)
    spacing: Style.buttonPadding
    padding: Style.buttonPadding
    leftPadding: 12
    rightPadding: 16

    font {
        family: control.font.family
        pointSize: Style.fontPointSize
    }

    icon {
        width: Style.buttonHeight - padding * 2
        // height: 24
        color: control.palette.buttonText
    }

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        alignment: Qt.AlignLeft
        icon: control.icon
        text: control.text
        font: control.font
        color: control.palette.buttonText
    }

    background: Rectangle {
        readonly property color baseColor: control.highlighted ? Style.backgroundAppendColor : "transparent"
        implicitWidth: 40
        implicitHeight: Style.buttonHeight
        color: control.down ? control.palette.highlight : baseColor
    }

}
