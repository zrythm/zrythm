// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import ZrythmStyle 1.0

Rectangle {
  readonly property color baseColor: control.highlighted ? control.palette.dark : control.palette.button
  readonly property color colorAdjustedForChecked: control.checked ? control.palette.accent : baseColor
  readonly property color colorAdjustedForHoverOrFocusOrDown: ZrythmTheme.adjustColorForHoverOrVisualFocusOrDown(colorAdjustedForChecked, control.hovered, control.visualFocus, control.down)
  required property var control
   readonly property real styleHeight: ZrythmTheme.buttonHeight

  border.color: control.palette.highlight
  border.width: control.visualFocus || control.down ? 2 : 0
  color: colorAdjustedForHoverOrFocusOrDown
  implicitHeight: styleHeight
  implicitWidth: styleHeight
  radius: ZrythmTheme.buttonRadius
  visible: !control.flat || control.down || control.checked || control.highlighted

  Behavior on border.width {
    animation: ZrythmTheme.propertyAnimation
  }
  Behavior on color {
    animation: ZrythmTheme.propertyAnimation
  }
}
