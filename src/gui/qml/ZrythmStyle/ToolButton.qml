// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import ZrythmStyle 1.0

T.ToolButton {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
    padding: Style.buttonPadding
    spacing: Style.buttonPadding
    hoverEnabled: true

    font: Style.buttonTextFont
    opacity: Style.getOpacity(control.enabled, control.Window.active)

    icon {
        // set only one dimension, let other be determined automatically
        width: Style.buttonHeight - 2 * control.padding
        // height: Style.buttonHeight - 2 * control.padding
        color: control.visualFocus || control.down ? control.palette.highlight : control.palette.buttonText
    }

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        icon: control.icon
        text: control.text
        font: control.font
        color: control.visualFocus || control.down ? control.palette.highlight : control.palette.buttonText
    }

    background: Rectangle {
        implicitWidth: Style.buttonHeight
        implicitHeight: Style.buttonHeight
        opacity: control.down ? 1 : (control.hovered ? 1 : 0.5)
        color: control.down || control.checked || control.highlighted ? control.palette.mid : control.palette.button
        radius: Style.buttonRadius
    }

}
