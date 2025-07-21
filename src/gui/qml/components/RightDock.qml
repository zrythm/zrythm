// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ColumnLayout {
  id: root

  TabBar {
    id: tabBar

    Layout.fillWidth: true

    TabButton {
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "shapes-large-symbolic.svg")

      ToolTip {
        text: qsTr("Plugin Browser")
      }
    }

    TabButton {
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "file-cabinet-symbolic.svg")

      ToolTip {
        text: qsTr("File Browser")
      }
    }

    TabButton {
      icon.source: ResourceManager.getIconUrl("zrythm-dark", "speaker.svg")

      ToolTip {
        text: qsTr("Monitor Section")
      }
    }

    TabButton {
      icon.source: ResourceManager.getIconUrl("zrythm-dark", "minuet-chords.svg")

      ToolTip {
        text: qsTr("Chord Preset Browser")
      }
    }
  }

  StackLayout {
    Layout.fillHeight: true
    Layout.fillWidth: true
    currentIndex: tabBar.currentIndex

    PluginBrowserPage {
      id: pluginBrowserPage

      pluginManager: GlobalState.zrythm.pluginManager
    }

    Repeater {
      model: 3

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
