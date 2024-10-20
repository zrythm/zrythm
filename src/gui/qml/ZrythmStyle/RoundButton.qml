// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import ZrythmStyle 1.0

T.RoundButton {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
    padding: 4
    horizontalPadding: padding + 2
    spacing: 4
    font: Style.buttonTextFont
    opacity: Style.getOpacity(control.enabled, control.Window.active)

    TextMetrics {
        id: textMetrics

        font: control.font
        text: "Test"
    }

    icon {
        // console.log("text metrics height: " + textMetrics.height);

        width: Math.max(Style.buttonHeight - padding * 2, textMetrics.height)
        // height: 24
        color: control.checked || control.highlighted ? control.palette.brightText : control.flat && !control.down ? (control.visualFocus ? control.palette.highlight : control.palette.windowText) : control.palette.buttonText
        Component.onCompleted: {
        }
    }

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        icon: control.icon
        text: control.text
        font: control.font
        color: control.checked || control.highlighted ? control.palette.brightText : control.flat && !control.down ? (control.visualFocus ? control.palette.highlight : control.palette.windowText) : control.palette.buttonText
    }

    background: Rectangle {
        readonly property color baseColor: control.highlighted ? control.palette.dark : control.palette.button
        readonly property color colorAdjustedForChecked: control.checked ? control.palette.accent : baseColor
        readonly property color colorAdjustedForHoverOrFocusOrDown: Style.adjustColorForHoverOrVisualFocusOrDown(colorAdjustedForChecked, control.hovered, control.visualFocus, control.down)

        radius: control.radius
        implicitWidth: Style.buttonHeight
        implicitHeight: Style.buttonHeight
        visible: !control.flat || control.down || control.checked || control.highlighted
        color: colorAdjustedForHoverOrFocusOrDown
        border.color: control.palette.highlight
        border.width: control.visualFocus || control.down ? 2 : 0

        Behavior on border.width {
            animation: Style.propertyAnimation
        }

        Behavior on color {
            animation: Style.propertyAnimation
        }

    }

}
