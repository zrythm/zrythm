// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Templates as T
import ZrythmStyle

T.ToolTip {
  id: control

  closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutsideParent | T.Popup.CloseOnReleaseOutsideParent
  delay: Style.toolTipDelay
  font.pointSize: Style.fontPointSize
  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)
  implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)
  margins: Style.buttonPadding
  padding: Style.buttonPadding
  visible: parent ? parent.hovered : false // qmllint disable missing-property
  x: parent ? (parent.width - implicitWidth) / 2 : 0
  y: -implicitHeight - 3

  background: PopupBackgroundRect {
  }
  contentItem: Text {
    color: Style.colorPalette.toolTipText
    font: control.font
    text: control.text
    wrapMode: Text.Wrap
  }
}
