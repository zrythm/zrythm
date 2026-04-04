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
  required property TrackSelectionModel trackSelectionModel
  required property Tracklist tracklist
  required property UndoStack undoStack

  Repeater {
    id: allChannels

    Layout.fillHeight: true

    delegate: ChannelView {
      audioEngine: root.audioEngine
      channel: track.channel
      pluginImporter: root.pluginImporter
      pluginOperator: root.pluginOperator
      trackSelectionModel: root.trackSelectionModel
      tracklist: root.tracklist
      undoStack: root.undoStack
    }
    model: SortFilterProxyModel {
      id: mixerProxyModel

      model: root.tracklist.collection

      filters: [
        FunctionFilter {
          function filter(data: TrackRoleData): bool {
            return root.tracklist.shouldBeVisible(data.track) && root.tracklist.hasChannel(data.track);
          }
        }
      ]
    }
  }

  component TrackRoleData: QtObject {
    required property Track track
  }
}
