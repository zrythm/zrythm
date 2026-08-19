// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

ColumnLayout {
  id: root

  required property PluginImporter pluginImporter
  required property Project project

  implicitWidth: Math.max(tabBar.implicitWidth, stackLayout.children[stackLayout.currentIndex]?.implicitWidth ?? 0)

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

    // TODO: enable after implementing chord presets
    // TabButton {
    //   icon.source: ResourceManager.getIconUrl("zrythm-dark", "minuet-chords.svg")

    //   ToolTip {
    //     text: qsTr("Chord Preset Browser")
    //   }
    // }
  }

  StackLayout {
    id: stackLayout

    Layout.fillHeight: true
    Layout.fillWidth: true
    currentIndex: tabBar.currentIndex

    PluginBrowserPage {
      id: pluginBrowserPage

      appSettings: GlobalState.application.appSettings
      pluginManager: GlobalState.application.pluginManager

      // TODO: import to current track
      onPluginDescriptorActivated: descriptor => root.pluginImporter.importPluginToNewTrack(descriptor)
    }

    FileBrowserPage {
      id: fileBrowserPage

      fileSystemModel: GlobalState.application.fileSystemModel
    }

    MonitorSection {
      id: monitorSection

      trackCollection: root.project.tracklist.collection
    }
  }
}
