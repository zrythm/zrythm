// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import ZrythmStyle 1.0

T.TextField {
  id: control

  color: control.palette.text
  font: Style.normalTextFont
  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding, placeholder.implicitHeight + topPadding + bottomPadding)
  implicitWidth: implicitBackgroundWidth + leftInset + rightInset || Math.max(contentWidth, placeholder.implicitWidth) + leftPadding + rightPadding
  leftPadding: padding + 4
  opacity: Style.getOpacity(control.enabled, control.Window.active)
  padding: Style.buttonPadding
  placeholderTextColor: control.palette.placeholderText
  selectedTextColor: control.palette.highlightedText
  selectionColor: control.palette.highlight
  verticalAlignment: TextInput.AlignVCenter

  background: Rectangle {
    color: control.palette.base
    implicitHeight: Style.buttonHeight
    implicitWidth: 200
    radius: Style.textFieldRadius

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
      color: control.activeFocus ? control.palette.highlight : control.palette.mid
      width: control.activeFocus ? 2 : 1
    }
  }

  PlaceholderText {
    id: placeholder

    color: control.placeholderTextColor
    elide: Text.ElideRight
    font: control.font
    height: control.height - (control.topPadding + control.bottomPadding)
    renderType: control.renderType
    text: control.placeholderText
    verticalAlignment: control.verticalAlignment
    visible: !control.length && !control.preeditText && (!control.activeFocus || control.horizontalAlignment !== Qt.AlignHCenter)
    width: control.width - (control.leftPadding + control.rightPadding)
    x: control.leftPadding
    y: control.topPadding
  }
}
