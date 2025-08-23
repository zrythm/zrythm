// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import ZrythmStyle 1.0

Rectangle {
  readonly property color baseColor: control.highlighted ? control.palette.dark : control.palette.button
  readonly property color colorAdjustedForChecked: control.checked ? control.palette.accent : baseColor
  readonly property color colorAdjustedForHoverOrFocusOrDown: Style.adjustColorForHoverOrVisualFocusOrDown(colorAdjustedForChecked, control.hovered, control.visualFocus, control.down)
  required property var control
   readonly property real styleHeight: Style.buttonHeight

  border.color: control.palette.highlight
  border.width: control.visualFocus || control.down ? 2 : 0
  color: colorAdjustedForHoverOrFocusOrDown
  implicitHeight: styleHeight
  implicitWidth: styleHeight
  radius: Style.buttonRadius
  visible: !control.flat || control.down || control.checked || control.highlighted

  Behavior on border.width {
    animation: Style.propertyAnimation
  }
  Behavior on color {
    animation: Style.propertyAnimation
  }
}
