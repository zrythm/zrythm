// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm

Item {
  id: root

  required property DeviceManager deviceManager
  required property PluginImporter pluginImporter
  required property PluginOperator pluginOperator
  required property ProjectSession session
  property Track track: null
  required property TrackSelectionModel trackSelectionModel
  required property UndoStack undoStack

  signal pluginClicked(Plugin plugin)

  function updateTrack(modelIndex: var): void {
    if (modelIndex === null || !modelIndex.valid)
      return;
    const selectedTrack = trackSelectionModel.getTrackFromModelIndex(modelIndex);
    if (selectedTrack !== null)
      track = selectedTrack;
  }

  implicitHeight: contentLoader.implicitHeight
  implicitWidth: contentLoader.implicitWidth

  Component.onCompleted: updateTrack(trackSelectionModel.currentIndex)

  Loader {
    id: contentLoader

    active: root.track !== null
    anchors.fill: parent

    sourceComponent: TrackInspectorPage {
      anchors.fill: parent
      audioEngine: root.session.project.engine
      deviceManager: root.deviceManager
      pluginImporter: root.pluginImporter
      pluginOperator: root.pluginOperator
      session: root.session
      track: root.track
      trackSelectionModel: root.trackSelectionModel
      undoStack: root.undoStack

      onPluginClicked: function (plugin: Plugin) {
        root.pluginClicked(plugin);
      }
    }
  }

  Connections {
    function onCurrentChanged(current: var, previous: var) {
      root.updateTrack(current);
    }

    target: root.trackSelectionModel
  }
}
