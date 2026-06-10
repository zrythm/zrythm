// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import ZrythmStyle 1.0

T.MenuBarItem {
  id: control

  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding, implicitIndicatorHeight + topPadding + bottomPadding)
  implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
  leftPadding: 12
  padding: ZrythmTheme.buttonPadding
  rightPadding: 16
  spacing: ZrythmTheme.buttonPadding

  background: Rectangle {
    readonly property color baseColor: control.highlighted ? ZrythmTheme.backgroundAppendColor : "transparent"

    color: control.down ? control.palette.highlight : baseColor
    implicitHeight: ZrythmTheme.buttonHeight
    implicitWidth: 40
  }
  contentItem: IconLabel {
    alignment: Qt.AlignLeft
    color: control.palette.buttonText
    display: control.display
    font: control.font
    icon: control.icon
    mirrored: control.mirrored
    spacing: control.spacing
    text: control.text
  }

  font {
    family: control.font.family
    pointSize: ZrythmTheme.fontPointSize
  }

  icon {
    // height: 24
    color: control.palette.buttonText
    width: ZrythmTheme.buttonHeight - padding * 2
  }
}
