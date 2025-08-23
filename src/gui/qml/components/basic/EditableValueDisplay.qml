// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Zrythm 1.0
import ZrythmStyle 1.0

Control {
  id: control

  property string label
  property real minValueHeight: 0
  property real minValueWidth: 0
  property alias value: valueDisplay.text

  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
  implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
  opacity: Style.getOpacity(control.enabled, control.Window.active)
  padding: 2

  contentItem: RowLayout {
    id: rowLayout

    implicitHeight: Math.max(valueDisplay.implicitHeight, textDisplay.implicitHeight)
    implicitWidth: valueDisplay.implicitWidth + textDisplay.implicitWidth + spacing
    spacing: 3

    Text {
      id: valueDisplay

      Layout.alignment: Qt.AlignBaseline
      Layout.fillWidth: true
      Layout.minimumHeight: control.minValueHeight
      Layout.minimumWidth: control.minValueWidth
      color: control.palette.text
      font: Style.semiBoldTextFont
      horizontalAlignment: Text.AlignHCenter
      textFormat: Text.PlainText

      Behavior on text {
        animation: Style.propertyAnimation
      }
    }

    Text {
      id: textDisplay

      Layout.alignment: Qt.AlignBaseline
      color: control.palette.placeholderText
      font: Style.fadedTextFont
      text: control.label
    }
  }
}
