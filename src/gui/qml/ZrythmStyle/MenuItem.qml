// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import ZrythmStyle 1.0

T.MenuItem {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding, implicitIndicatorHeight + topPadding + bottomPadding)
    padding: Style.buttonPadding
    spacing: Style.buttonPadding
    icon.width: Style.buttonHeight - 2 * control.padding
    // icon.height: 24
    icon.color: Style.colorPalette.windowText
    font: Style.semiBoldTextFont

    contentItem: IconLabel {
        readonly property real arrowPadding: control.subMenu && control.arrow ? control.arrow.width + control.spacing : 0
        readonly property real indicatorPadding: control.checkable && control.indicator ? control.indicator.width + control.spacing : 0

        leftPadding: !control.mirrored ? indicatorPadding : arrowPadding
        rightPadding: control.mirrored ? indicatorPadding : arrowPadding
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        alignment: Qt.AlignLeft
        icon: control.icon
        text: control.text
        font: control.font
        color: control.highlighted ? Style.colorPalette.highlightedText : Style.colorPalette.windowText
    }

    indicator: ColorImage {
        x: control.mirrored ? control.width - width - control.rightPadding : control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        visible: control.checked
        source: control.checkable ? "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png" : ""
        color: Style.colorPalette.windowText
        defaultColor: "#353637"
        sourceSize.width: icon.width
        sourceSize.height: icon.width
    }

    arrow: ColorImage {
        x: control.mirrored ? control.leftPadding : control.width - width - control.rightPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        visible: control.subMenu
        mirror: control.mirrored
        source: control.subMenu ? "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/arrow-indicator.png" : ""
        color: Style.colorPalette.windowText
        defaultColor: "#353637"
        sourceSize.width: icon.width - 4
        sourceSize.height: icon.width - 4
    }

    background: Rectangle {
        readonly property color baseColor: control.highlighted ? Style.colorPalette.highlight : Style.colorPalette.button
        readonly property color colorAdjustedForHoverOrFocusOrDown: Style.adjustColorForHoverOrVisualFocusOrDown(baseColor, /*control.hovered*/ false, control.visualFocus, control.down)

        implicitWidth: 200
        implicitHeight: Style.buttonHeight
        x: 1
        y: 1
        width: control.width - 2
        height: control.height - 2
        color: colorAdjustedForHoverOrFocusOrDown

        Behavior on color {
            animation: Style.propertyAnimation
        }

    }

}
