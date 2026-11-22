// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick

Item {
  id: root

  required property color accentColor
  property color highlightColor: Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.35)
  property string zone: ""

  opacity: zone.length > 0 ? 1 : 0
  visible: opacity > 0

  Behavior on opacity {
    OpacityAnimator {
      duration: 100
    }
  }

  Rectangle {
    border.color: root.accentColor
    border.width: 2
    color: root.highlightColor
    height: root.zone === "top" || root.zone === "bottom" ? root.height / 2 : root.height
    width: root.zone === "left" || root.zone === "right" ? root.width / 2 : root.width
    x: root.zone === "right" ? root.width / 2 : 0
    y: root.zone === "bottom" ? root.height / 2 : 0
  }
}
