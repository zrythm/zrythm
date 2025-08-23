// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import ZrythmStyle 1.0

T.Button {
  id: control

  property real styleHeight: Style.buttonHeight

  font: Style.buttonTextFont
  horizontalPadding: padding + 2
  hoverEnabled: true
  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
  implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
  layer.enabled: true
  opacity: Style.getOpacity(control.enabled, control.Window.active)
  padding: 4
  spacing: 4

  background: ButtonBackgroundRect {
    control: control
  }
  contentItem: IconLabel {
    readonly property color baseColor: control.highlighted ? control.palette.brightText : control.palette.buttonText
    readonly property color colorAdjustedForChecked: control.checked ? control.palette.brightText : baseColor
    readonly property color colorAdjustedForHoverOrFocusOrDown: Style.adjustColorForHoverOrVisualFocusOrDown(colorAdjustedForChecked, false, false, control.down)

    color: colorAdjustedForHoverOrFocusOrDown
    display: control.display
    font: control.font
    icon: control.icon
    mirrored: control.mirrored
    spacing: control.spacing
    text: control.text

    Behavior on color {
      animation: Style.propertyAnimation
    }
  }
  layer.effect: DropShadowEffect {
  }

  TextMetrics {
    id: textMetrics

    font: control.font
    text: "Test"
  }

  icon {
    // height: 24
    color: control.checked || control.highlighted ? control.palette.brightText : control.flat && !control.down ? (control.visualFocus ? control.palette.highlight : control.palette.windowText) : control.palette.buttonText
    width: Math.max(control.styleHeight - padding * 2, textMetrics.height)
  }
}
