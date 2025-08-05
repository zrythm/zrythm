// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import ZrythmStyle 1.0

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
