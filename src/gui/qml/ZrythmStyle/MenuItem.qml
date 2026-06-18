// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import ZrythmStyle 1.0

T.MenuItem {
  id: control

  font: ZrythmTheme.semiBoldTextFont
  // icon.height: 24
  icon.color: ZrythmTheme.colorPalette.windowText
  icon.width: ZrythmTheme.buttonHeight - 2 * control.padding
  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding, implicitIndicatorHeight + topPadding + bottomPadding)
  implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
  padding: ZrythmTheme.buttonPadding
  spacing: ZrythmTheme.buttonPadding

  arrow: ColorImage {
    color: ZrythmTheme.colorPalette.windowText
    defaultColor: "#353637"
    mirror: control.mirrored
    source: control.subMenu ? "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/arrow-indicator.png" : ""
    sourceSize.height: control.icon.width - 4
    sourceSize.width: control.icon.width - 4
    visible: control.subMenu
    x: control.mirrored ? control.leftPadding : control.width - width - control.rightPadding
    y: control.topPadding + (control.availableHeight - height) / 2
  }
  background: Rectangle {
    readonly property color baseColor: control.highlighted ? ZrythmTheme.colorPalette.highlight : ZrythmTheme.colorPalette.button
    readonly property color colorAdjustedForHoverOrFocusOrDown: ZrythmTheme.adjustColorForHoverOrVisualFocusOrDown(baseColor, false, control.visualFocus, control.down) //control.hovered

    color: colorAdjustedForHoverOrFocusOrDown
    height: control.height - 2
    implicitHeight: ZrythmTheme.buttonHeight
    implicitWidth: 200
    width: control.width - 2
    x: 1
    y: 1

    Behavior on color {
      animation: ZrythmTheme.propertyAnimation
    }
  }
  contentItem: IconLabel {
    readonly property real arrowPadding: control.subMenu && control.arrow ? control.arrow.width + control.spacing : 0
    readonly property real indicatorPadding: control.checkable && control.indicator ? control.indicator.width + control.spacing : 0

    alignment: Qt.AlignLeft
    color: control.highlighted ? ZrythmTheme.colorPalette.highlightedText : ZrythmTheme.colorPalette.windowText
    display: control.display
    font: control.font
    icon: control.icon
    leftPadding: !control.mirrored ? indicatorPadding : arrowPadding
    mirrored: control.mirrored
    rightPadding: control.mirrored ? indicatorPadding : arrowPadding
    spacing: control.spacing
    text: control.text
  }
  indicator: ColorImage {
    color: ZrythmTheme.colorPalette.windowText
    defaultColor: "#353637"
    source: control.checkable ? "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png" : ""
    sourceSize.height: control.icon.width
    sourceSize.width: control.icon.width
    visible: control.checked
    x: control.mirrored ? control.width - width - control.rightPadding : control.leftPadding
    y: control.topPadding + (control.availableHeight - height) / 2
  }
}
