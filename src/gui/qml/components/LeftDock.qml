// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

ColumnLayout {

  id: root
  required property Tracklist tracklist

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

    TrackInspectorPage {
      track: root.tracklist.selectedTrack
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
