// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Dialogs
import Zrythm
import ZrythmDocking

Item {
  id: root

  required property AppSettings appSettings
  required property PlaybackCacheActivityAggregator cacheActivityAggregator
  required property ChordPresetManager chordPresetManager
  required property DeviceManager deviceManager
  readonly property DockingController dockingController: workspace.dockingController
  required property Palette dockPalette
  required property FileDialog importFileDialog
  readonly property Project project: session.project
  property Plugin selectedPlugin: null
  required property ProjectSession session
  required property TrackSelectionModel trackSelectionModel

  signal focusItemChanged(Item focusItem)

  function resetLayout(): void {
    appSettings.leftPanelVisible = true;
    appSettings.bottomPanelVisible = true;
    appSettings.rightPanelVisible = true;
    appSettings.reset_dockLayout();
    workspace.dockingController.resetLayout();
  }

  function saveLayout(): void {
    appSettings.dockLayout = workspace.dockingController.saveState();
  }

  function revealArea(area: string): void {
    switch (area) {
    case "start":
      appSettings.leftPanelVisible = true;
      break;
    case "bottom":
      appSettings.bottomPanelVisible = true;
      break;
    case "end":
      appSettings.rightPanelVisible = true;
      break;
    }
  }

  function showPluginInspector(plugin: Plugin): void {
    selectedPlugin = plugin;
    workspace.dockingController.presentPanel("plugin-inspector");
  }

  DockWorkspace {
    id: workspace

    anchors.fill: parent
    bottomRevealed: root.appSettings.bottomPanelVisible
    defaultLayout: ({
      "version": 1,
      "main": {
        "areas": {
          "start": {
            "id": "left",
            "kind": "frame",
            "panels": ["track-inspector", "plugin-inspector"],
            "active": "track-inspector",
            "showLabels": false
          },
          "end": {
            "id": "right",
            "kind": "frame",
            "panels": ["plugin-browser", "file-browser", "monitor"],
            "active": "plugin-browser",
            "showLabels": false
          },
          "top": null,
          "bottom": {
            "id": "bottom",
            "kind": "frame",
            "panels": ["editor", "mixer", "chord-pad"],
            "active": "editor",
            "tabsAtBottom": true
          },
          "center": {
            "id": "center",
            "kind": "frame",
            "panels": ["arrangement", "modular"],
            "active": "arrangement",
            "tabsAtBottom": true
          }
        },
        "sizes": { "start": 200, "end": 200, "top": 200, "bottom": 240 }
      },
      "floating": [],
      "hidden": []
    })
    dockPalette: root.dockPalette
    endRevealed: root.appSettings.rightPanelVisible
    initialLayout: root.appSettings.dockLayout
    startRevealed: root.appSettings.leftPanelVisible

    onAreaRevealRequested: area => root.revealArea(area)

    DockPanel {
      defaultFrame: "left"
      icon: ResourceManager.getIconUrl("zrythm-dark", "track-inspector.svg")
      panelId: "track-inspector"
      title: qsTr("Track Inspector")

      TrackInspectorPanel {
        anchors.fill: parent
        deviceManager: root.deviceManager
        pluginImporter: root.session.pluginImporter
        pluginOperator: root.session.pluginOperator
        session: root.session
        trackSelectionModel: root.trackSelectionModel
        undoStack: root.session.undoStack

        onPluginClicked: plugin => root.selectedPlugin = plugin
      }
    }

    DockPanel {
      defaultFrame: "left"
      icon: ResourceManager.getIconUrl("zrythm-dark", "plug.svg")
      panelId: "plugin-inspector"
      title: qsTr("Plugin Inspector")

      Loader {
        active: root.selectedPlugin !== null
        anchors.fill: parent

        sourceComponent: PluginInspectorPage {
          anchors.fill: parent
          plugin: root.selectedPlugin
        }
      }
    }

    DockPanel {
      defaultFrame: "right"
      icon: ResourceManager.getIconUrl("gnome-icon-library", "shapes-large-symbolic.svg")
      panelId: "plugin-browser"
      title: qsTr("Plugin Browser")

      PluginBrowserPage {
        anchors.fill: parent
        appSettings: root.appSettings
        pluginManager: GlobalState.application.pluginManager

        onPluginDescriptorActivated: descriptor => root.session.pluginImporter.importPluginToNewTrack(descriptor)
      }
    }

    DockPanel {
      defaultFrame: "right"
      icon: ResourceManager.getIconUrl("gnome-icon-library", "file-cabinet-symbolic.svg")
      panelId: "file-browser"
      title: qsTr("File Browser")

      FileBrowserPage {
        anchors.fill: parent
        fileSystemModel: GlobalState.application.fileSystemModel
      }
    }

    DockPanel {
      defaultFrame: "right"
      icon: ResourceManager.getIconUrl("zrythm-dark", "speaker.svg")
      panelId: "monitor"
      title: qsTr("Monitor Section")

      MonitorSection {
        anchors.fill: parent
        trackCollection: root.project.tracklist.collection
      }
    }

    DockPanel {
      defaultFrame: "bottom"
      icon: ResourceManager.getIconUrl("zrythm-dark", "piano-roll.svg")
      panelId: "editor"
      title: qsTr("Editor")

      ClipEditorPanel {
        anchors.fill: parent
        session: root.session
      }
    }

    DockPanel {
      defaultFrame: "bottom"
      icon: ResourceManager.getIconUrl("zrythm-dark", "mixer.svg")
      panelId: "mixer"
      title: qsTr("Mixer")

      MixerView {
        anchors.fill: parent
        audioEngine: root.project.engine
        pluginImporter: root.session.pluginImporter
        pluginOperator: root.session.pluginOperator
        portObservationManager: root.project.portObservationManager
        trackSelectionModel: root.trackSelectionModel
        tracklist: root.project.tracklist
        undoStack: root.session.undoStack

        onPluginClicked: plugin => root.showPluginInspector(plugin)
      }
    }

    DockPanel {
      defaultFrame: "bottom"
      icon: ResourceManager.getIconUrl("zrythm-dark", "chord-pad.svg")
      panelId: "chord-pad"
      title: qsTr("Chord Pad")

      ChordPadPanel {
        anchors.fill: parent
        chordPresetManager: root.chordPresetManager
        session: root.session
      }
    }

    DockPanel {
      defaultFrame: "center"
      icon: ResourceManager.getIconUrl("zrythm-dark", "roadmap.svg")
      panelId: "arrangement"
      title: qsTr("Arrangement")

      ArrangementPanel {
        anchors.fill: parent
        cacheActivityAggregator: root.cacheActivityAggregator
        importFileDialog: root.importFileDialog
        session: root.session
        trackSelectionModel: root.trackSelectionModel
      }
    }

    DockPanel {
      defaultFrame: "center"
      icon: ResourceManager.getIconUrl("gnome-icon-library", "connected-dots-symbolic.svg")
      panelId: "modular"
      title: qsTr("Modular")

      ModularView {
        anchors.fill: parent
        session: root.session
        trackSelectionModel: root.trackSelectionModel
      }
    }
  }

  Connections {
    function onFocusItemChanged(focusItem: Item) {
      root.focusItemChanged(focusItem);
    }

    target: workspace.dockingController
  }

  Connections {
    function onInspectorRequested(plugin: Plugin) {
      root.showPluginInspector(plugin);
    }

    target: PluginInspectorController
  }
}
