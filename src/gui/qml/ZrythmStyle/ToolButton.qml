// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import ZrythmStyle 1.0

// assume flat and non-highlighted - other uses not supported
T.ToolButton {
    id: control

    function getTextColor() : color {
        let c = control.palette.buttonText;
        if (control.checked)
            c = control.palette.highlight;

        if (!control.hovered && !control.focused && !control.down)
            return c;
        else if (control.down)
            return Style.getStrongerColor(c);
        else if (hovered || focused)
            return c;
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
    padding: Style.buttonPadding
    spacing: Style.buttonPadding
    hoverEnabled: true
    flat: true
    font: Style.buttonTextFont
    opacity: Style.getOpacity(control.enabled, control.Window.active)

    TextMetrics {
        id: textMetrics

        font: control.font
        text: "Test"
    }

    icon {
        width: Style.buttonHeight - 2 * control.padding
        height: Style.buttonHeight - 2 * control.padding
        color: control.getTextColor()

        Behavior on color {
            animation: Style.propertyAnimation
        }

    }

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        icon: control.icon
        text: control.text
        font: control.font
        color: control.getTextColor()

        Behavior on color {
            animation: Style.propertyAnimation
        }

    }

    background: Rectangle {
        implicitWidth: Style.buttonHeight
        implicitHeight: Style.buttonHeight
        color: {
            let c = Qt.color("transparent");
            if (control.hovered)
                c = Style.buttonHoverBackgroundAppendColor;

            if (control.down) {
                if (Style.isColorDark(c))
                    c.lighter(Style.lightenFactor);
                else
                    c.darker(Style.lightenFactor);
            }
            return c;
        }
        radius: Style.toolButtonRadius

        border {
            width: control.visualFocus ? 2 : 0
            color: control.palette.highlight
        }

        Behavior on color {
            animation: Style.propertyAnimation
        }

        Behavior on border.width {
            animation: Style.propertyAnimation
        }

    }

}
