// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQml.Models
import Zrythm

ListView {
  id: root

  required property AudioEngine audioEngine
  required property bool pinned
  required property TrackSelectionModel trackSelectionModel
  required property Tracklist tracklist
  required property UndoStack undoStack

  Layout.fillWidth: true
  boundsBehavior: Flickable.StopAtBounds
  clip: true
  delegateModelAccess: DelegateModel.ReadWrite
  implicitHeight: 200
  implicitWidth: 200

  delegate: TrackView {
    id: trackView

    required property int index

    audioEngine: root.audioEngine
    trackIndex: root.pinned ? index : index + root.tracklist.pinnedTracksCutoff
    trackSelectionModel: root.trackSelectionModel
    tracklist: root.tracklist
    undoStack: root.undoStack
    width: ListView.view.width

    Binding on height {
      value: 0
      when: !trackView.track.visible
    }
  }
  model: SortFilterProxyModel {
    id: proxyModel

    model: root.tracklist.collection

    filters: [
      FunctionFilter {
        function filter(data: TrackRoleData): bool {
          return root.tracklist.isTrackPinned(data.track) === root.pinned;
        }
      }
    ]
  }

  Connections {
    function onPinnedTracksCutoffChanged() {
      proxyModel.invalidate();
    }

    target: root.tracklist
  }

  component TrackRoleData: QtObject {
    property Track track
  }
}
