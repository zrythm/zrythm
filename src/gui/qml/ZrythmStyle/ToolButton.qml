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

  function getTextColor(): color {
    let c = control.palette.buttonText;
    if (control.checked)
      c = control.palette.highlight;

    if (!control.hovered && !control.focus && !control.down)
      return c;
    else if (control.down)
      return Style.getStrongerColor(c);
    else if (hovered || focus)
      return c;
  }

  flat: true
  font: Style.buttonTextFont
  hoverEnabled: true
  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
  implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
  opacity: Style.getOpacity(control.enabled, control.Window.active)
  padding: Style.buttonPadding
  spacing: Style.buttonPadding

  background: Rectangle {
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
    implicitHeight: Style.buttonHeight
    implicitWidth: Style.buttonHeight
    radius: Style.toolButtonRadius

    Behavior on border.width {
      animation: Style.propertyAnimation
    }
    Behavior on color {
      animation: Style.propertyAnimation
    }

    border {
      color: control.palette.highlight
      width: control.visualFocus ? 2 : 0
    }
  }
  contentItem: IconLabel {
    color: control.getTextColor()
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

  TextMetrics {
    id: textMetrics

    font: control.font
    text: "Test"
  }

  icon {
    color: control.getTextColor()
    height: Style.buttonHeight - 2 * control.padding
    width: Style.buttonHeight - 2 * control.padding

    Behavior on color {
      animation: Style.propertyAnimation
    }
  }
}
