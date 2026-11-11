// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick.Layouts
import QtQuick
import QtQml.Models
import Zrythm

RowLayout {
  id: root

  required property AudioEngine audioEngine
  required property PluginImporter pluginImporter
  required property PluginOperator pluginOperator
  required property PortObservationManager portObservationManager
  required property ProcessorParameterOperator parameterOperator
  required property Project project
  required property TrackSelectionModel trackSelectionModel
  required property Tracklist tracklist
  required property UndoStack undoStack

  signal pluginClicked(Plugin plugin)

  Repeater {
    id: allChannels

    Layout.fillHeight: true

    delegate: ChannelView {
      audioEngine: root.audioEngine
      channel: track.channel
      parameterOperator: root.parameterOperator
      pluginImporter: root.pluginImporter
      pluginOperator: root.pluginOperator
      portObservationManager: root.portObservationManager
      project: root.project
      trackSelectionModel: root.trackSelectionModel
      tracklist: root.tracklist
      undoStack: root.undoStack

      onPluginClicked: function(plugin: Plugin) {
        root.pluginClicked(plugin);
      }
    }
    model: SortFilterProxyModel {
      id: mixerProxyModel

      sourceModel: root.tracklist.collection

      filters: [
        FunctionFilter {
          function filter(track: Track): bool {
            return root.tracklist.shouldBeVisible(track) && root.tracklist.hasChannel(track);
          }
        }
      ]
    }
  }
}
