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

  required property PluginImporter pluginImporter
  required property PluginOperator pluginOperator
  required property Project project
  property Plugin selectedPlugin: null
  required property TrackSelectionModel trackSelectionModel
  required property Tracklist tracklist
  required property UndoStack undoStack

  Connections {
    function onInspectorRequested(plugin) {
      root.selectedPlugin = plugin;
      tabBar.currentIndex = 1;
    }

    target: PluginInspectorController
  }

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

      property Track track: null

      Layout.fillHeight: false
      Layout.fillWidth: true
      active: track !== null
      visible: tabBar.currentIndex === 0 && active

      sourceComponent: TrackInspectorPage {
        anchors.fill: parent
        audioEngine: root.project.engine
        pluginImporter: root.pluginImporter
        pluginOperator: root.pluginOperator
        track: trackInspectorLoader.track
        trackSelectionModel: root.trackSelectionModel
        undoStack: root.undoStack

        onPluginClicked: function (plugin: Plugin) {
          root.selectedPlugin = plugin;
        }
      }

      Connections {
        function onCurrentChanged(current, previous) {
          if (current) {
            const track = root.trackSelectionModel.getTrackFromModelIndex(current);
            if (track) {
              trackInspectorLoader.track = track;
            }
          }
        }

        target: root.trackSelectionModel
      }
    }

    Loader {
      id: pluginInspectorLoader

      Layout.fillHeight: false
      Layout.fillWidth: true
      active: root.selectedPlugin !== null
      visible: tabBar.currentIndex === 1 && active

      sourceComponent: PluginInspectorPage {
        plugin: root.selectedPlugin
      }
    }
  }
}
