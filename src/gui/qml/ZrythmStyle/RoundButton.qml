// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import ZrythmStyle 1.0

T.RoundButton {
  id: control

  property real styleHeight: Style.buttonHeight

  font: Style.buttonTextFont
  horizontalPadding: padding + 2
  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
  implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
  opacity: Style.getOpacity(control.enabled, control.Window.active)
  padding: 4
  spacing: 4

  background: Rectangle {
    readonly property color baseColor: control.highlighted ? control.palette.dark : control.palette.button
    readonly property color colorAdjustedForChecked: control.checked ? control.palette.accent : baseColor
    readonly property color colorAdjustedForHoverOrFocusOrDown: Style.adjustColorForHoverOrVisualFocusOrDown(colorAdjustedForChecked, control.hovered, control.visualFocus, control.down)

    border.color: control.palette.highlight
    border.width: control.visualFocus || control.down ? 2 : 0
    color: colorAdjustedForHoverOrFocusOrDown
    implicitHeight: control.styleHeight
    implicitWidth: control.styleHeight
    radius: control.radius
    visible: !control.flat || control.down || control.checked || control.highlighted

    Behavior on border.width {
      animation: Style.propertyAnimation
    }
    Behavior on color {
      animation: Style.propertyAnimation
    }
  }
  contentItem: IconLabel {
    color: control.checked || control.highlighted ? control.palette.brightText : control.flat && !control.down ? (control.visualFocus ? control.palette.highlight : control.palette.windowText) : control.palette.buttonText
    display: control.display
    font: control.font
    icon: control.icon
    mirrored: control.mirrored
    spacing: control.spacing
    text: control.text
  }

  TextMetrics {
    id: textMetrics

    font: control.font
    text: "Test"
  }

  icon {
    // height: 24
    color: control.checked || control.highlighted ? control.palette.brightText : control.flat && !control.down ? (control.visualFocus ? control.palette.highlight : control.palette.windowText) : control.palette.buttonText
    // console.log("text metrics height: " + textMetrics.height);

    width: Math.max(control.styleHeight - padding * 2, textMetrics.height)

    Component.onCompleted: {}
  }
}
