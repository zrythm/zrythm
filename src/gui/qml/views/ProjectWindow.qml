// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import "../config.js" as Config
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ApplicationWindow {
  id: root

  property Arranger activeArranger: null
  readonly property Action deleteAction: Action {
    id: deleteAction

    enabled: root.activeArranger !== null
    shortcut: StandardKey.Delete
    text: qsTr("&Delete")

    onTriggered: {
      if (root.activeArranger) {
        root.activeArranger.selectionOperator.deleteObjects(root.project.tracklist);
      }
    }
  }
  required property DeviceManager deviceManager
  readonly property Action fullScreenAction: Action {
    id: fullScreenAction

    shortcut: StandardKey.FullScreen
    text: qsTr("Fullscreen")

    onTriggered: {
      root.visibility = root.visibility === Window.FullScreen ? Window.AutomaticVisibility : Window.FullScreen;
    }
  }
  readonly property Project project: projectUiState.project
  required property ProjectUiState projectUiState
  required property AppSettings appSettings

  function closeAndDestroy() {
    console.log("Closing and destroying project window");
    close();
    destroy();
  }

  height: 720
  title: project.title
  visible: true
  width: 1280

  header: MainToolbar {
    id: headerBar

    projectUiState: root.projectUiState
    appSettings: root.appSettings
  }
  menuBar: MainMenuBar {
    id: mainMenuBar

    aboutDialog: aboutDialog
    deviceManager: root.deviceManager
    exportDialog: exportDialog
    project: root.project
  }

  Component.onCompleted: {
    console.log("ApplicationWindow created on platform", Qt.platform.os);
    project.aboutToBeDeleted.connect(closeAndDestroy);
    project.activate();
  }
  onClosing: {
    console.log("Application shutting down");
    closeAndDestroy();
    Qt.quit();
  }

  Connections {
    function onRowsAboutToBeRemoved(modelIndex: var, first: int, last: int) {
      // Auto-deselect tracks when removed from the project
      for (let i = first; i <= last; ++i) {
        const trackModelIndex = trackSelectionModel.getModelIndex(i);
        trackSelectionModel.select(trackModelIndex, ItemSelectionModel.Deselect);
      }
    }

    function onRowsInserted(modelIndex: var, first: int, last: int) {
      // Auto-select tracks when added to the project
      const trackModelIndex = trackSelectionModel.getModelIndex(last);
      trackSelectionModel.selectSingleTrack(trackModelIndex);
    }

    function onRowsRemoved(modelIndex: var, first: int, last: int) {
      // Ensure at least 1 track is always selected
      if (!trackSelectionModel.hasSelection) {
        const numTracks = root.project.tracklist.collection.rowCount();

        // Select next track, or prev track if next doesn't exist
        let indexToSelect = first;
        console.log(first, indexToSelect);
        if (numTracks === first) {
          --indexToSelect;
        }

        const trackModelIndex = trackSelectionModel.getModelIndex(indexToSelect);
        trackSelectionModel.selectSingleTrack(trackModelIndex);
      }
    }

    target: root.project.tracklist.collection
  }

  AboutDialog {
    id: aboutDialog

  }

  ExportDialog {
    id: exportDialog

    project: root.project
  }

  // A unified collection of selected plugins and plugin containers (which contain other plugins or plugin containers)
  UnifiedProxyModel {
    id: unifiedPluginsModel

  }

  ItemSelectionModel {
    id: pluginSelectionModel

    // Returns a PluginGroup or a Plugin
    function getObjectFromUnifiedIndex(unifiedIndex: var): var {
      const sourceIndex = unifiedPluginsModel.mapToSource(unifiedIndex);
      return sourceIndex.data(PluginGroup.DeviceGroupPtrRole);
    }

    model: unifiedPluginsModel

    onCurrentChanged: (current, previous) => {
      if (current) {
        const pluginOrGroup = getObjectFromUnifiedIndex(current);
        if (pluginOrGroup instanceof Plugin) {
          const plugin = pluginOrGroup as Plugin;
          console.log("current plugin changed to", plugin.configuration.descriptor.name);
        }
      }
    }
    onSelectionChanged: (selected, deselected) => {
      console.log("Selection changed:", selectedIndexes.length, "items selected");
      if (selectedIndexes.length > 0) {
        const firstPluginOrGroup = selectedIndexes[0].data(PluginGroup.DeviceGroupPtrRole);
        console.log("first selected plugin/group:", firstPluginOrGroup);
      }

      if (deselected.length > 0) {
        deselected.forEach(deselectedRange => {
          const pluginOrGroup = getObjectFromUnifiedIndex(deselectedRange.topLeft);
          if (pluginOrGroup instanceof Plugin) {
            const plugin = pluginOrGroup as Plugin;
            console.log("previous plugin changed");
          }
        });
      }
    }
  }

  TrackSelectionModel {
    id: trackSelectionModel

    // Map to track which tracks were automatically armed for recording
    // Key: track object, Value: boolean indicating if recording was set automatically
    property var automaticallyArmedTracks: ({})

    model: root.project.tracklist.collection

    Component.onCompleted: {
      // Select last track
      const numTracks = root.project.tracklist.collection.rowCount();
      if (numTracks > 0) {
        const trackModelIndex = getModelIndex(numTracks - 1);
        selectSingleTrack(trackModelIndex);
      }
    }
    onCurrentChanged: (current, previous) => {
      if (current) {
        const track = getTrackFromModelIndex(current);
        if (track) {
          console.log("current track changed", track.name);
        }
      }
    }
    onSelectionChanged: (selected, deselected) => {
      console.log("Selection changed:", selectedIndexes.length, "items selected");
      if (selectedIndexes.length > 0) {
        const firstTrack = selectedIndexes[0].data(TrackCollection.TrackPtrRole) as Track;
        console.log("first selected object:", firstTrack.name);

        selected.forEach(selectedRange => {
          for (let i = selectedRange.topLeft; i <= selectedRange.bottomRight; i++) {
            const track = getTrackFromModelIndex(i);
            if (track.recordableTrackMixin) {
              // Auto-arm management
              if (root.appSettings.trackAutoArm) {
                const rec = track.recordableTrackMixin;
                if (!rec?.recording) {
                  rec.recording = true;
                  automaticallyArmedTracks[track] = true;
                }
              }
            }
          }
        });
      }

      if (deselected.length > 0) {
        deselected.forEach(deselectedRange => {
          for (let i = deselectedRange.topLeft; i <= deselectedRange.bottomRight; i++) {
            const track = getTrackFromModelIndex(i);
            if (track.recordableTrackMixin) {
              // Auto-arm management for deselected tracks
              if (root.appSettings.trackAutoArm && automaticallyArmedTracks[track]) {
                const rec = track.recordableTrackMixin;
                rec.recording = false;
                delete automaticallyArmedTracks[track];
              }
            }
          }
        });
      }
    }
  }

  Shortcut {
    context: Qt.ApplicationShortcut
    enabled: root.projectUiState.undoStack && root.projectUiState.undoStack.canUndo
    sequences: [StandardKey.Undo]

    onActivated: root.projectUiState.undoStack.undo()
  }

  Shortcut {
    context: Qt.ApplicationShortcut
    enabled: root.projectUiState.undoStack && root.projectUiState.undoStack.canRedo
    sequences: [StandardKey.Redo]

    onActivated: root.projectUiState.undoStack.redo()
  }

  // Global spacebar for play/pause
  Shortcut {
    context: Qt.ApplicationShortcut
    sequence: "Space"

    onActivated: {
      // Toggle play/pause regardless of focus
      if (root.project.transport.isRolling()) {
        root.project.transport.requestPause();
      } else {
        root.project.transport.requestRoll();
      }
    }
  }

  Shortcut {
    context: Qt.ApplicationShortcut
    sequences: [StandardKey.Save]

    onActivated: {
      console.log("save requested");
      // TODO
      // root.project.save();
    }
  }

  // Tools
  Shortcut {
    sequence: "1"

    onActivated: {
      root.projectUiState.tool.toolValue = ArrangerTool.Select;
    }
  }

  Shortcut {
    sequence: "2"

    onActivated: {
      root.projectUiState.tool.toolValue = ArrangerTool.Edit;
    }
  }

  Shortcut {
    sequence: "3"

    onActivated: {
      root.projectUiState.tool.toolValue = ArrangerTool.Cut;
    }
  }

  Shortcut {
    sequence: "4"

    onActivated: {
      root.projectUiState.tool.toolValue = ArrangerTool.Eraser;
    }
  }

  Shortcut {
    sequence: "5"

    onActivated: {
      root.projectUiState.tool.toolValue = ArrangerTool.Ramp;
    }
  }

  Shortcut {
    sequence: "6"

    onActivated: {
      root.projectUiState.tool.toolValue = ArrangerTool.Audition;
    }
  }

  ColumnLayout {
    anchors.fill: parent
    spacing: 0

    SplitView {
      id: mainSplitView

      Layout.fillHeight: true
      Layout.fillWidth: true
      orientation: Qt.Horizontal

      LeftDock {
        SplitView.fillHeight: true
        SplitView.minimumWidth: 40
        SplitView.preferredWidth: 200
        pluginImporter: root.projectUiState.pluginImporter
        project: root.project
        trackSelectionModel: trackSelectionModel
        tracklist: root.project.tracklist
        undoStack: root.projectUiState.undoStack
        visible: root.appSettings.leftPanelVisible
      }

      SplitView {
        id: centerSplitView

        SplitView.fillHeight: true
        SplitView.fillWidth: true
        SplitView.minimumWidth: 120
        orientation: Qt.Vertical

        CenterDock {
          Layout.verticalStretchFactor: 2
          SplitView.fillHeight: true
          SplitView.fillWidth: true
          SplitView.minimumHeight: implicitHeight
          SplitView.preferredHeight: 200
          projectUiState: root.projectUiState
          trackSelectionModel: trackSelectionModel
        }

        BottomDock {
          Layout.verticalStretchFactor: 1
          SplitView.fillHeight: true
          SplitView.fillWidth: true
          SplitView.minimumHeight: 40
          SplitView.preferredHeight: 240
          projectUiState: root.projectUiState
          visible: root.appSettings.bottomPanelVisible
        }
      }

      RightDock {
        id: rightDock

        SplitView.fillHeight: true
        SplitView.minimumWidth: 30
        SplitView.preferredWidth: 200
        pluginImporter: root.projectUiState.pluginImporter
        project: root.project
        visible: root.appSettings.rightPanelVisible
      }
    }

    Item {
      id: botBar

      Layout.fillWidth: true
      implicitHeight: 24

      Text {
        anchors.right: parent.right
        font: root.font
        text: "Status Bar"
      }
    }
  }
}
