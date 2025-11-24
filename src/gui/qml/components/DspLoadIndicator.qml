// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import ZrythmStyle

Control {
  id: root

  readonly property color criticalLoadColor: "#E53935"
  readonly property color highLoadColor: "#F44336"
  property double loadValue: 0.0
  readonly property color lowLoadColor: palette.text //"#4CAF50"
  readonly property color mediumLoadColor: "#FF9800"
  property int xRuns: 0

  function getLoadColor(load) {
    return load < 50 ? lowLoadColor : load < 75 ? mediumLoadColor : load < 90 ? highLoadColor : criticalLoadColor;
  }

  implicitHeight: 24
  implicitWidth: 8

  ToolTip {
    text: qsTr("DSP Load: %1%\nXRun Count: %2").arg(Math.round(root.loadValue)).arg(root.xRuns)
  }

  // Vertical progress bar
  Rectangle {
    anchors.centerIn: parent
    border.color: root.palette.mid
    border.width: 1
    color: root.palette.base
    height: 20
    radius: 2
    width: 6

    Rectangle {
      anchors.bottom: parent.bottom
      color: root.getLoadColor(root.loadValue)
      height: parent.height * (root.loadValue / 100.0)
      radius: parent.radius
      width: parent.width

      Behavior on height {
        NumberAnimation {
          duration: 150
        }
      }
    }
  }
}
