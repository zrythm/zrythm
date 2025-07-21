// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ColumnLayout {
  TabBar {
    id: tabBar

    Layout.fillWidth: true

    TabButton {
      icon.source: ResourceManager.getIconUrl("zrythm-dark", "track-inspector.svg")

      ToolTip {
        text: qsTr("Track Inspector")
      }
    }

    TabButton {
      icon.source: ResourceManager.getIconUrl("zrythm-dark", "plug.svg")

      ToolTip {
        text: qsTr("Plugin Inspector")
      }
    }
  }

  StackLayout {
    Layout.fillHeight: true
    Layout.fillWidth: true
    currentIndex: tabBar.currentIndex

    ZrythmResizablePanel {
      title: "Browser"
      vertical: false

      content: Rectangle {
        color: "#2C2C2C"

        Label {
          anchors.centerIn: parent
          color: "white"
          text: "Browser content"
        }
      }
    }

    Repeater {
      model: 1

      Rectangle {
        border.color: "black"
        border.width: 2
        color: Qt.rgba(Math.random(), Math.random(), Math.random(), 1)
        height: 50
        width: 180

        Text {
          anchors.centerIn: parent
          font.pixelSize: 16
          text: "Item " + (index + 1)
        }
      }
    }
  }
}
