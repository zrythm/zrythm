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
      return ZrythmTheme.getStrongerColor(c);
    else if (hovered || focus)
      return c;
  }

  flat: true
  font: ZrythmTheme.buttonTextFont
  hoverEnabled: true
  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
  implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
  opacity: ZrythmTheme.getOpacity(control.enabled, control.Window.active)
  padding: ZrythmTheme.buttonPadding
  spacing: ZrythmTheme.buttonPadding

  background: Rectangle {
    color: {
      let c = Qt.color("transparent");
      if (control.hovered)
        c = ZrythmTheme.buttonHoverBackgroundAppendColor;

      if (control.down) {
        if (ZrythmTheme.isColorDark(c))
          c.lighter(ZrythmTheme.lightenFactor);
        else
          c.darker(ZrythmTheme.lightenFactor);
      }
      return c;
    }
    implicitHeight: ZrythmTheme.buttonHeight
    implicitWidth: ZrythmTheme.buttonHeight
    radius: ZrythmTheme.toolButtonRadius

    Behavior on border.width {
      animation: ZrythmTheme.propertyAnimation
    }
    Behavior on color {
      animation: ZrythmTheme.propertyAnimation
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
      animation: ZrythmTheme.propertyAnimation
    }
  }

  TextMetrics {
    id: textMetrics

    font: control.font
    text: "Test"
  }

  icon {
    color: control.getTextColor()
    height: ZrythmTheme.buttonHeight - 2 * control.padding
    width: ZrythmTheme.buttonHeight - 2 * control.padding

    Behavior on color {
      animation: ZrythmTheme.propertyAnimation
    }
  }
}
