// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import ZrythmStyle 1.0

T.ItemDelegate {
  id: control

  font: Style.semiBoldTextFont
  icon.color: control.palette.text
  icon.height: 24
  icon.width: 24
  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding, implicitIndicatorHeight + topPadding + bottomPadding)
  implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
  padding: 4
  spacing: 4

  background: Rectangle {
    readonly property color baseColor: control.highlighted ? control.palette.highlight : control.palette.button
    readonly property color colorAdjustedForHoverOrFocusOrDown: Style.adjustColorForHoverOrVisualFocusOrDown(baseColor, false, control.visualFocus, control.down) //control.hovered

    color: colorAdjustedForHoverOrFocusOrDown
    implicitHeight: Style.buttonHeight
    implicitWidth: 100
    visible: control.down || control.highlighted || control.visualFocus

    Behavior on color {
      animation: Style.propertyAnimation
    }
  }
  contentItem: IconLabel {
    alignment: control.display === IconLabel.IconOnly || control.display === IconLabel.TextUnderIcon ? Qt.AlignCenter : Qt.AlignLeft
    color: control.highlighted ? control.palette.highlightedText : control.palette.text
    display: control.display
    font: control.font
    icon: control.icon
    mirrored: control.mirrored
    spacing: control.spacing
    text: control.text
  }
}
