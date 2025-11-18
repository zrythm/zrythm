// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

ColumnLayout {
  id: root

  required property Project project
  required property Tracklist tracklist
  required property UndoStack undoStack

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

    Loader {
      id: trackInspectorLoader

      property Track track: root.project.trackSelectionManager.lastSelectedObject

      Layout.fillHeight: false
      Layout.fillWidth: true
      active: track !== null
      visible: active

      sourceComponent: TrackInspectorPage {
        anchors.fill: parent
        track: trackInspectorLoader.track
        undoStack: root.undoStack
        pluginImporter: root.project.pluginImporter
      }

      Connections {
        function onLastSelectedObjectChanged() {
          trackInspectorLoader.track = root.project.trackSelectionManager.lastSelectedObject;
        }

        target: root.project.trackSelectionManager
      }
    }

    Rectangle {
      Layout.fillHeight: true
      Layout.fillWidth: true
      border.color: "black"
      border.width: 2
      color: Qt.rgba(Math.random(), Math.random(), Math.random(), 1)

      Text {
        anchors.centerIn: parent
        font.pixelSize: 16
        text: "Item "
      }
    }
  }
}
