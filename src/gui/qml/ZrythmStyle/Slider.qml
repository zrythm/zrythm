// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T
import ZrythmStyle 1.0

T.Slider {
  id: control

  hoverEnabled: true
  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitHandleHeight + topPadding + bottomPadding)
  implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitHandleWidth + leftPadding + rightPadding)
  opacity: ZrythmTheme.getOpacity(control.enabled, control.Window.active)
  padding: 3

  background: Rectangle {
    id: groove

    color: control.palette.text
    height: control.horizontal ? implicitHeight : control.availableHeight
    implicitHeight: control.horizontal ? 6 : 200
    implicitWidth: control.horizontal ? 200 : 6
    radius: 3
    scale: control.horizontal && control.mirrored ? -1 : 1
    width: control.horizontal ? control.availableWidth : implicitWidth
    x: control.leftPadding + (control.horizontal ? 0 : (control.availableWidth - width) / 2)
    y: control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : 0)

    // Accent fill up to the handle. Mirrored (RTL) sliders flip the whole
    // groove so the fill hugs the trailing edge; vertical sliders grow the
    // fill from the bottom edge.
    Rectangle {
      id: fill

      objectName: "fill"
      color: control.palette.highlight
      height: control.horizontal ? groove.height : groove.height - y
      radius: 3
      width: control.horizontal ? control.position * groove.width : groove.width
      x: 0
      y: control.horizontal ? 0 : control.visualPosition * groove.height
    }
  }
  handle: Rectangle {
    border.color: control.visualFocus ? control.palette.highlight : ZrythmTheme.polarityTextColor(control.palette.highlight)
    border.width: control.visualFocus ? 2 : 1
    color: ZrythmTheme.adjustColorForHoverOrVisualFocusOrDown(control.palette.highlight, control.hovered, control.visualFocus, control.pressed)
    implicitHeight: 18
    implicitWidth: 18
    radius: width / 2
    x: control.leftPadding + (control.horizontal ? control.visualPosition * (control.availableWidth - width) : (control.availableWidth - width) / 2)

    // Vertical sliders travel bottom-up (fader convention): the minimum
    // value sits at the bottom edge. visualPosition measures top-down in
    // vertical orientation, so this places minimum at the bottom.
    y: control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : control.visualPosition * (control.availableHeight - height))

    Behavior on color {
      animation: ZrythmTheme.propertyAnimation
    }
  }
}
