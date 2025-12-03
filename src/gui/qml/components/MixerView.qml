// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick.Layouts
import QtQuick
import Zrythm

RowLayout {
  id: root

  required property AudioEngine audioEngine
  required property PluginImporter pluginImporter
  required property Tracklist tracklist
  required property UndoStack undoStack

  Repeater {
    id: allChannels

    Layout.fillHeight: true

    delegate: ChannelView {
      audioEngine: root.audioEngine
      channel: track.channel
      pluginImporter: root.pluginImporter
      tracklist: root.tracklist
      undoStack: root.undoStack
    }
    model: TrackFilterProxyModel {
      sourceModel: root.tracklist.collection

      Component.onCompleted: {
        addVisibilityFilter(true);
        addChannelFilter(true);
      }
    }
  }
}
