// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic.impl
import QtQuick.Templates as T
import ZrythmStyle 1.0

T.ProgressBar {
  id: control

  implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
  implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
  opacity: Style.getOpacity(control.enabled, control.Window.active)

  background: Rectangle {
    color: control.palette.text
    implicitHeight: 6
    implicitWidth: 200
    // y: (control.height - height) / 2
    // height: 6
    radius: 3
  }
  contentItem: Item {
    implicitHeight: 6
    implicitWidth: 200

    // Progress indicator for determinate state.
    Rectangle {
      color: palette.highlight
      height: parent.height
      radius: 4
      visible: !control.indeterminate
      width: control.visualPosition * parent.width
    }

    // Scrolling animation for indeterminate state.
    Item {
      anchors.fill: parent
      clip: true
      visible: control.indeterminate

      Row {
        anchors.fill: parent
        spacing: 0

        Repeater {
          model: 5

          Rectangle {
            id: rectangle

            color: control.palette.highlight
            height: control.height
            radius: 4
            width: control.width / 3

            NumberAnimation on x {
              duration: 1500
              easing.type: Easing.InOutQuad
              from: -rectangle.width
              loops: Animation.Infinite
              to: control.width
            }
          }
        }
      }
    }
  }
}
