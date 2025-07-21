// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import ZrythmStyle 1.0

T.TabButton {
  id: control

  readonly property color iconLabelColor: control.checked ? control.palette.brightText : control.palette.buttonText

  font: Style.semiBoldTextFont
  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
  implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
  opacity: Style.getOpacity(control.enabled, control.Window.active)
  padding: 4
  spacing: 4

  background: Rectangle {
    readonly property bool isBeginning: control.TabBar.index === 0
    readonly property bool isEnd: control.TabBar.index === control.TabBar.tabBar.count - 1
    readonly property real radiusToUse: height / 2

    bottomLeftRadius: isBeginning ? radiusToUse : 0
    bottomRightRadius: isEnd ? radiusToUse : 0
    color: {
      let c = control.checked ? control.palette.highlight : control.palette.button;
      if (control.down)
        return Style.getStrongerColor(c);
      else if (control.visualFocus || (control.hovered && !control.checked))
        return Style.getColorBlendedTowardsContrast(c);
      return c;
    }
    implicitHeight: Style.buttonHeight
    topLeftRadius: isBeginning ? radiusToUse : 0
    topRightRadius: isEnd ? radiusToUse : 0

    Behavior on border.color {
      animation: Style.propertyAnimation
    }
    Behavior on border.width {
      animation: Style.propertyAnimation
    }
    Behavior on color {
      animation: Style.propertyAnimation
    }

    border {
      color: control.palette.highlight
      width: control.visualFocus ? 2 : 0
    }
  }
  contentItem: IconLabel {
    color: control.iconLabelColor
    display: control.display
    font: control.font
    icon: control.icon
    mirrored: control.mirrored
    spacing: control.spacing
    text: control.text
  }

  icon {
    color: iconLabelColor
    height: Style.buttonHeight - 2 * padding
    width: Style.buttonHeight - 2 * padding
  }
}
