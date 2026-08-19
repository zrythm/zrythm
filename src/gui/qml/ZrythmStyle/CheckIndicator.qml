// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls.impl
import ZrythmStyle

/**
 * Rounded-square check indicator with a check glyph when checked.
 *
 * Shared by CheckBox and any control that presents a checkable state.
 */
Rectangle {
  id: root

  readonly property color baseColor: root.checked ? root.palette.accent : root.palette.base
  property bool checked: false
  property bool down: false
  property bool hovered: false
  property bool visualFocus: false

  color: ZrythmTheme.adjustColorForHoverOrVisualFocusOrDown(baseColor, root.hovered, false, root.down)
  implicitHeight: 16
  implicitWidth: 16
  radius: ZrythmTheme.textFieldRadius

  Behavior on color {
    animation: ZrythmTheme.propertyAnimation
  }

  border {
    color: root.visualFocus ? root.palette.highlight : (root.checked ? root.palette.accent : root.palette.mid)
    width: root.visualFocus ? 2 : 1
  }

  ColorImage {
    anchors.centerIn: parent
    color: root.palette.brightText
    fillMode: Image.PreserveAspectFit
    height: 12
    source: "qrc:/qt/qml/Zrythm/icons/noto-glyphs/check.svg"
    visible: root.checked
    width: 12
  }
}
