// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
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
  model: TrackFilterProxyModel {
    sourceModel: root.tracklist.collection

    Component.onCompleted: {
      addPinnedFilter(root.pinned);
    }
  }
}
